/**
 * @file storage.cpp
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


#include "storage.h"
#include <Shlwapi.h>
#include <UserEnv.h>

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "userenv")


namespace hackernewscmd {
	const std::string Storage::kFilename = "hackernewscmd.dat";

	Storage::Storage(std::string&& filepath, const Key&) {
		char buffer[MAX_PATH];
		::PathCombineA(buffer, filepath.c_str(), kFilename.c_str());
		mFilepath = std::string(buffer);
		ReadSkippedStoryIds();
	}

	Storage::~Storage() {
		WriteSkippedStoryIds();
	}

	std::unordered_set<StoryId>* Storage::GetSkippedStoryIds() {
		return &mSkippedStoryIds;
	}

	std::unique_ptr<Storage> Storage::mInstance = nullptr;
	Storage& Storage::GetInstance() {
		if (mInstance == nullptr) {
			HANDLE tokenHandle;
			if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS, &tokenHandle) == FALSE) {
				throw std::runtime_error("Couldn't get process token for current process");
			}
			unsigned long bufSize = MAX_PATH;
			char buffer[MAX_PATH];
			if (::GetUserProfileDirectoryA(tokenHandle, buffer, &bufSize) == FALSE) {
				::CloseHandle(tokenHandle); // Ignore error
				throw std::runtime_error("Couldn't get user profile directory");
			}
			::CloseHandle(tokenHandle); // Ignore error
			mInstance = std::make_unique<Storage>(std::string(buffer), Key{});
		}
		return *mInstance;
	}

	void Storage::ReadSkippedStoryIds() {
		std::fstream stream(mFilepath, std::fstream::in);

		if (!stream) {
			return;
		}

		try {
			unsigned long countSkipped;
			stream >> countSkipped;
			StoryId val;
			while (countSkipped-- > 0) {
				stream >> val;
				mSkippedStoryIds.insert(val);
			}
		} catch (const std::ios_base::failure&) {
			stream.close();
		}
	}

	void Storage::WriteSkippedStoryIds() {
		std::fstream stream(mFilepath, std::fstream::out | std::fstream::trunc);

		if (!stream) {
			return;
		}

		try {
			stream << mSkippedStoryIds.size();
			for (const auto& id : mSkippedStoryIds) {
				stream << " " << id;
			}
		} catch (const std::ios_base::failure&) {
			stream.close();
		}
	}
}