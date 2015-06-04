/**
 * @file input_manager.cpp
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


#include "input_manager.h"


namespace hackernewscmd {
	InputManager::InputManager(const Interact& interact, StateManager& sm) :
		mInteract(interact),
		mStateManager(sm),
		mInputState(InputState::Empty) {};

	InputManager::~InputManager() {
		if (mInputThread.joinable()) {
			mInputThread.detach();
		}
	}

	void InputManager::Go() {
		mInputThread = std::thread(&InputManager::ThreadCallback, this);
	}

	void InputManager::Wait() {
		if (mInputThread.joinable()) {
			mInputThread.join();
		}
	}

	void InputManager::ThreadCallback() {
		for (;;) {
			ProcessActions(mInteract.ReadActions());
			if (mInputState == InputState::Quit) {
				break;
			}
		}
	}

	void InputManager::ProcessActions(const std::vector<InputAction>& actions) {
		using IA = InputAction;
		for (const auto action : actions) {
			switch (action) {
			case IA::NextStory:
				mStateManager.SelectNextStory(false);
				break;
			case IA::NextStorySkip:
				mStateManager.SelectNextStory(true);
				break;
			case IA::PrevStory:
				mStateManager.SelectPrevStory(false);
				break;
			case IA::PrevStorySkip:
				mStateManager.SelectPrevStory(true);
				break;
			case IA::NextPage:
				mStateManager.GotoNextPage(false);
				break;
			case IA::NextPageSkip:
				mStateManager.GotoNextPage(true);
				break;
			case IA::PrevPage:
				mStateManager.GotoPrevPage(false);
				break;
			case IA::PrevPageSkip:
				mStateManager.GotoPrevPage(true);
				break;
			case IA::OpenStory:
				mStateManager.OpenSelectedStoryUrl();
				break;
			case IA::RefreshStories:
				// TODO
				break;
			case IA::Quit:
				mInputState = InputState::Quit;
				mStateManager.Quit();
				return;
			}
		}
	}
} // namespace hackernewscmd