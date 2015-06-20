/**
 * @file state_manager.cpp
 *
 * Copyright 2015 Mayank Kumar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "state_manager.h"
#include <algorithm>
#include <chrono>
#include <future>
#include <stdexcept>
#include <utility>

#undef min


namespace hackernewscmd {
	StateManager::StateManager():
		mCurrentDisplayPage(-1),
		mCurrentSelectedStoryIndex(0),
		mIsInited(false),
		mSkippedStories(nullptr),
		mDisplayMutex(std::mutex()),
		mDisplayLock(mDisplayMutex, std::defer_lock),
		mDisplayReverseMutex(std::mutex()),
		mDisplayReverseLock(mDisplayReverseMutex, std::defer_lock) {};

	void StateManager::Init(Storage& storage, NewsFetcher& fetcher, DisplayManager& dispManager) {
		mStorage = &storage;
		mFetcher = &fetcher;
		mDisplayManager = &dispManager;
		mIsInited = true;
	}

	void StateManager::Start() {
		if (!mIsInited) {
			throw std::runtime_error("StateManager has not been initialized");
		}

		auto loadFromStorage = std::async(&StateManager::LoadFromStorage, this);
		auto loadTopStories = std::async(&StateManager::FetchTopStories, this);
		loadFromStorage.wait();
		loadTopStories.wait();
		DiffTopStories();

		mPagedDisplayBuffer.resize(mTopStories.size());

		mCurrentDisplayPage = 0;
		mCurrentSelectedStoryIndex = 0;

		PageIndices indices;
		TryGetIndicesForDisplayPage(0, indices); // First page, no need to check for result
		FetchDisplayPage(indices);

		SetupDisplayThreadDataForPageDisplay(indices, 1);
		mDisplayPageData.totalPages = (mTopStories.size() - 1) / kDisplayPageSize;
		mDisplayManager->Go(mDisplayCV, *(mDisplayLock.mutex()), mDisplayThreadData, mDisplayReverseCV);
	}

	void StateManager::GotoNextPage(bool skipCurr) {
		GotoPage(mCurrentDisplayPage + 1, skipCurr);
	}

	void StateManager::GotoPrevPage(bool skipCurr) {
		GotoPage(mCurrentDisplayPage - 1, skipCurr);
	}

	void StateManager::SelectNextStory(bool skipCurr) {
		SelectStory(mCurrentSelectedStoryIndex + 1, skipCurr);
	}

	void StateManager::SelectPrevStory(bool skipCurr) {
		SelectStory(mCurrentSelectedStoryIndex - 1, skipCurr);
	}

	void StateManager::OpenSelectedStory(bool shouldOpenComments) {
		const wchar_t* url = nullptr;
		std::wstring urlComments;

		auto& story = mPagedDisplayBuffer[mCurrentSelectedStoryIndex].first;
		if (!shouldOpenComments && story.url.length()) {
			url = story.url.c_str();
		} else {
			urlComments = GetStoryPageUrl(story);
			url = urlComments.c_str();
		}
		if (reinterpret_cast<int>(::ShellExecuteW(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL)) <= 32) {
			throw std::runtime_error("Couldn't open browser");
		}

		if (!shouldOpenComments) {
			mSkippedStories->insert(story.id);
		}
	}

	void StateManager::Quit() {
		mDisplayLock.lock();
		mDisplayThreadData.redo = true;
		mDisplayThreadData.action = DisplayThreadData::Quit;
		mDisplayLock.unlock();
		mDisplayCV.notify_all();
		mDisplayManager->Wait();
	}

	std::unique_ptr<StateManager> StateManager::mInstance = nullptr;
	StateManager& StateManager::GetInstance() {
		if (mInstance == nullptr) {
			mInstance = std::make_unique<StateManager>(Key{});
		}
		return *mInstance;
	}

	void StateManager::LoadFromStorage() {
		mSkippedStories = mStorage->GetSkippedStoryIds();
	}

	void StateManager::FetchTopStories() {
		mTopStories = mFetcher->FetchTopStoryIds();
	}

	void StateManager::DiffTopStories() {
		decltype(mTopStories) newTopStories;
		std::unordered_set<StoryId> newSkippedStories;
		for (const auto& story : mTopStories) {
			if (mSkippedStories->find(story) == mSkippedStories->end()) {
				newTopStories.emplace_back(story);
			} else {
				newSkippedStories.insert(story);
			}
		}
		mTopStories.swap(newTopStories);
		mSkippedStories->swap(newSkippedStories);
	}

	void StateManager::GotoPage(const long page, const bool skipCurr) {
		PageIndices indices;

		if (skipCurr) {
			TryGetIndicesForDisplayPage(mCurrentDisplayPage, indices);
			for (auto index = indices.first; index < indices.second; ++index) {
				mSkippedStories->insert(mTopStories[index]);
			}
		}

		if (TryGetIndicesForDisplayPage(page, indices)) {
			FetchDisplayPage(indices);
			DisplayPage(indices, page);
			mCurrentDisplayPage = page;
			SelectStory(indices.first, false);
		}
	}

	void StateManager::SelectStory(const std::size_t index, const bool skipCurr) {
		if (skipCurr) {
			mSkippedStories->insert(mTopStories[index]);
		}

		if (index < 0 || index >= mTopStories.size()) {
			return;
		}

		PageIndices indices;
		TryGetIndicesForDisplayPage(mCurrentDisplayPage, indices);
		if (index < indices.first || index >= indices.second) {
			// Load the required page
			GotoPage(index / kDisplayPageSize, false);
		} else if (mCurrentSelectedStoryIndex >= indices.first && mCurrentSelectedStoryIndex < indices.second) {
			// Don't move the selection if there'll be no indication on the console
			for (auto it = mPagedDisplayBuffer.cbegin() + indices.first; it != mPagedDisplayBuffer.cbegin() + indices.second; ++it) {
				if (it->second != StoryLoadStatus::Completed) {
					return;
				}
			}
		}

		// Wait if a prior instruction is pending a read by the display thread
		if (mDisplayThreadData.redo) {
			mDisplayReverseLock.lock();
			mDisplayReverseCV.wait(mDisplayReverseLock);
			mDisplayReverseLock.unlock();
		}

		SetupDisplayThreadDataForSelectedStory(index);
		mDisplayCV.notify_all();
		mCurrentSelectedStoryIndex = index;
	}

	const std::wstring StateManager::kHackerNewsItemUrl = L"https://news.ycombinator.com/item?id=";
	std::wstring StateManager::GetStoryPageUrl(const Story& story) {
		return kHackerNewsItemUrl + std::to_wstring(story.id);
	}

	void StateManager::FetchDisplayPage(const PageIndices& indices) {
		std::vector<std::pair<StoryId, size_t>> toBeLoadedTopStories;

		for (auto startIndex = indices.first; startIndex < indices.second; ++startIndex) {
			if (mPagedDisplayBuffer[startIndex].second == StoryLoadStatus::NotStarted) {
				toBeLoadedTopStories.push_back(std::make_pair(mTopStories[startIndex], startIndex));
			}
		}

		auto ftd = new FetchThreadData(std::move(toBeLoadedTopStories), std::move(std::bind(&StateManager::OnFetchStoryComplete, this, std::placeholders::_1, std::placeholders::_2)));
		std::async(&NewsFetcher::FetchStories, mFetcher, ftd);
	}

	void StateManager::DisplayPage(const PageIndices& indices, const long pageIndex) {
		SetupDisplayThreadDataForPageDisplay(indices, pageIndex + 1);
		mDisplayCV.notify_all();
	}

	void StateManager::SetupDisplayThreadDataForPageDisplay(const PageIndices& indices, const long currentPage) throw() {
		mDisplayLock.lock();
		mDisplayThreadData.redo = true;
		mDisplayThreadData.action = DisplayThreadData::DisplayPage;
		mDisplayPageData.begin = mPagedDisplayBuffer.cbegin() + indices.first;
		mDisplayPageData.end = mPagedDisplayBuffer.cbegin() + indices.second;
		mDisplayPageData.currentPage = currentPage;
		mDisplayThreadData.SetPointer(&mDisplayPageData);
		mDisplayLock.unlock();
	}

	bool StateManager::TryGetIndicesForDisplayPage(long page, PageIndices& result) const {
		auto begin = page * kDisplayPageSize;
		if (begin >= mPagedDisplayBuffer.size()) {
			return false;
		}

		result.first = begin;
		result.second = std::min(begin + kDisplayPageSize, static_cast<unsigned long>(mPagedDisplayBuffer.size()));
		return true;
	}

	void StateManager::SetupDisplayThreadDataForSelectedStory(const size_t index) {
		mDisplayLock.lock();
		mDisplayThreadData.redo = true;
		mDisplayThreadData.action = DisplayThreadData::SelectStory;
		mDisplayThreadData.SetPointer(&mPagedDisplayBuffer[index].first);
		mDisplayLock.unlock();
	}

	void StateManager::OnFetchStoryComplete(Story story, size_t index) {
		if (mPagedDisplayBuffer[index].second != StoryLoadStatus::Completed) {
			mPagedDisplayBuffer[index].first = std::move(story);
			mPagedDisplayBuffer[index].second = StoryLoadStatus::Completed;
			mDisplayCV.notify_all();
		}
	}
} // namespace hackernewscmd