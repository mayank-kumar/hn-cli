/**
 * @file display_manager.cpp
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


#include "display_manager.h"
#include <WinInet.h>


namespace hackernewscmd {
	DisplayManager::DisplayManager(const Interact& interact) :
		mInteract(interact),
		mCV(nullptr),
		mStateManagerCV(nullptr),
		mThreadData(nullptr),
		mCurrentlySelectedStory(nullptr),
		mToBeSelectedStory(nullptr),
		mShouldDisplayCommentCount(true) {};

	DisplayManager::~DisplayManager() {
		if (mDisplayThread.joinable()) {
			mDisplayThread.detach();
		}
		mLock.release();
	}

	void DisplayManager::Go(std::condition_variable& cv, std::mutex& m, DisplayThreadData& threadData, std::condition_variable& stateManagerCV) {
		mCV = &cv;
		mLock = std::unique_lock<std::mutex>(m, std::defer_lock);
		mThreadData = &threadData;
		mStateManagerCV = &stateManagerCV;
		mDisplayThread = std::thread(&DisplayManager::ThreadCallback, this);
	}

	void DisplayManager::Wait() {
		if (mDisplayThread.joinable()) {
			mDisplayThread.join();
		}
	}

	void DisplayManager::ThreadCallback() {
		using DTD = DisplayThreadData;

		mLock.lock();
		for (;;) {
			bool shouldRedo = mThreadData->redo = false; // Since we're beginning the process

			switch (mThreadData->action) {
			case DTD::DisplayPage: {
				mInteract.ClearScreen();
				mDisplayData.clear();
				auto data = mThreadData->GetActionData<DTD::DisplayPage, DTD::DisplayPageData>();
				mCurrentlySelectedStory = &data->begin->first;
				for (auto iter = data->begin; iter != data->end; ++iter) {
					while (iter->second != StoryLoadStatus::Completed) {
						mCV->wait(mLock);
					}
					if (TryReadNewInstruction() && (shouldRedo = ShouldBreak()) == true) {
						break;
					}
					auto& story = iter->first;
					if (mShouldDisplayCommentCount) {
						mDisplayData[story.id] = mInteract.ShowStory(story.title, story.score, GetHostNameFromUrl(story.url), story.descendants);
					} else {
						mDisplayData[story.id] = mInteract.ShowStory(story.title, story.score, GetHostNameFromUrl(story.url));
					}
				}
				if (shouldRedo) {
					break;
				}
				mInteract.ShowPagePosition(data->currentPage, data->totalPages);
				if (mToBeSelectedStory == nullptr) {
					mToBeSelectedStory = mCurrentlySelectedStory;
				}
				mInteract.SwapSelectedStories(mDisplayData[mCurrentlySelectedStory->id], mDisplayData[mToBeSelectedStory->id]);
				mCurrentlySelectedStory = mToBeSelectedStory;
				break;
			}
			case DTD::SelectStory: {
				auto data = mThreadData->GetActionData<DTD::SelectStory, Story>();
				if (mDisplayData.count(data->id)) {
					mInteract.SwapSelectedStories(mDisplayData[mCurrentlySelectedStory->id], mDisplayData[data->id]);
					mCurrentlySelectedStory = data;
				}
				break;
			}
			case DTD::Quit:
				mLock.unlock();
				return;
			default:
				break;
			}
			if (!shouldRedo) {
				mCV->wait(mLock);
				mStateManagerCV->notify_all();
			}
		}
	}

	bool DisplayManager::TryReadNewInstruction() {
		auto ret = mThreadData->redo;
		mThreadData->redo = false;
		if (ret) {
			if (mThreadData->action == DisplayThreadData::Action::SelectStory) {
				mToBeSelectedStory = mThreadData->GetActionData<DisplayThreadData::Action::SelectStory, Story>();
			}
			mStateManagerCV->notify_all();
		}
		return ret;
	}

	bool DisplayManager::ShouldBreak() const {
		auto action = mThreadData->action;
		return action == DisplayThreadData::Action::DisplayPage
			|| action == DisplayThreadData::Action::Quit;
	}

	std::wstring DisplayManager::GetHostNameFromUrl(const std::wstring& url)
	{
		URL_COMPONENTSW uc{};
		uc.dwStructSize = sizeof(uc);

		const auto buffer_size = 4096;
		wchar_t buff[buffer_size];
		uc.lpszHostName = buff;
		uc.dwHostNameLength = buffer_size;

		if (!::InternetCrackUrlW(url.c_str(), url.length(), ICU_DECODE, &uc)) {
			// Couldn't extract hostname
			return url;
		}

		return std::wstring(buff);
	}
} // namespace hackernewscmd