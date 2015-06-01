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


namespace hackernewscmd {
	DisplayManager::DisplayManager(const Interact& interact) :
		mInteract(interact),
		mCV(nullptr),
		mStateManagerCV(nullptr),
		mThreadData(nullptr),
		mCurrentlySelectedStory(nullptr),
		mToBeSelectedStory(nullptr) {};

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
					mDisplayData[iter->first] = mInteract.ShowStory(iter->first);
				}
				if (shouldRedo) {
					break;
				}
				if (mToBeSelectedStory == nullptr) {
					mToBeSelectedStory = mCurrentlySelectedStory;
				}
				mInteract.SwapSelectedStories(mDisplayData[*mCurrentlySelectedStory], mDisplayData[*mToBeSelectedStory]);
				mCurrentlySelectedStory = mToBeSelectedStory;
				break;
			}
			case DisplayThreadData::SelectStory: {
				auto data = mThreadData->GetActionData<DisplayThreadData::SelectStory, Story>();
				if (mDisplayData.count(*data)) {
					mInteract.SwapSelectedStories(mDisplayData[*mCurrentlySelectedStory], mDisplayData[*data]);
					mCurrentlySelectedStory = data;
				}
				break;
			}
			case DisplayThreadData::Quit:
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
} // namespace hackernewscmd