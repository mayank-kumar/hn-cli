/**
 * @file input_manager.h
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


#pragma once

#include <string>
#include <thread>
#include "interact.h"
#include "state_manager.h"


namespace hackernewscmd {
	class InputManager {
	public:
		InputManager(const Interact&, StateManager&);
		~InputManager();

		void Go();
		void Wait();

	private:
		enum class InputState {
			Empty,
			Quit
		};

		std::thread mInputThread;
		InputState mInputState;
		const Interact& mInteract;
		StateManager& mStateManager;

		void ThreadCallback();
		void ProcessInput(const std::wstring&);
	};
} // namespace hackernewscmd