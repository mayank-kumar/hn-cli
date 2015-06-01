/**
 * @file main.cpp
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


#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#include <stdexcept>
#include "display_manager.h"
#include "fetcher.h"
#include "input_manager.h"
#include "interact.h"
#include "state_manager.h"
#include "storage.h"

namespace hn = hackernewscmd;

int wmain(int, wchar_t*[])
{
	try {
		auto& interact = hn::Interact::GetInstance();
		auto& stateManager = hn::StateManager::GetInstance();
		hn::InputManager inputManager(interact, stateManager);
		hn::NewsFetcher newsFetcher;
		auto& storage = hn::Storage::GetInstance();
		hn::DisplayManager displayManager(interact);

		stateManager.Init(storage, newsFetcher, displayManager);

		inputManager.Go();
		stateManager.Start();

		inputManager.Wait();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
	}

	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
	return 0;
}
