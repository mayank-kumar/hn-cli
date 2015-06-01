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
			ProcessInput(mInteract.ReadChars());
			if (mInputState == InputState::Quit) {
				break;
			}
		}
	}

	void InputManager::ProcessInput(const std::wstring& input) {
		for (const auto c : input) {
			switch (::tolower(c)) {

			// Page navigation
			case 's':
				mStateManager.GotoNextPage(true);
				return;
			case '\t':
				mStateManager.GotoNextPage(false);
				return;
			case '\b':
				mStateManager.GotoPrevPage(false);
				return;

			// Story navigation
			case 'n':
				mStateManager.SelectNextStory(false);
				return;
			case 'p':
				mStateManager.SelectPrevStory(false);
				return;

			// Other controls
			case '\r':
				mStateManager.OpenSelectedStoryUrl();
				return;
			case 'q':
				mInputState = InputState::Quit;
				mStateManager.Quit();
				return;

			default:
				break;
			}
		}
	}
} // namespace hackernewscmd