/**
 * @file fetcher.h
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

#include <Windows.h>
#include <Wininet.h>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "story.h"

#pragma comment(lib, "Wininet")


namespace hackernewscmd {
	struct FetchThreadData {
		const std::vector<std::pair<StoryId, size_t>> ToBeLoaded;
		const std::function<void(Story, size_t)> OnFetchComplete;

		FetchThreadData(decltype(ToBeLoaded) && toBeLoaded, decltype(OnFetchComplete)&& onFetchComplete) :
			ToBeLoaded(std::move(toBeLoaded)),
			OnFetchComplete(std::move(onFetchComplete)) {};
		FetchThreadData& operator=(const FetchThreadData&) = delete;
	};

	class NewsFetcher {
	public:
		NewsFetcher();
		~NewsFetcher();
		std::vector<unsigned long long> FetchTopStoryIds();
		void FetchStories(const FetchThreadData*);

	private:
		HINTERNET mInternetHandle;
		PTP_POOL mThreadpool;
		PTP_CALLBACK_ENVIRON mThreadpoolCallbackEnvironment;
		static const unsigned long kMaxThreads = 5;
		static const std::string kBaseUrl;
		static const std::string kTopStories;

		HINTERNET GetInternetHandle();
		PTP_CALLBACK_ENVIRON GetThreadpoolCallbackEnvironment();
		std::vector<wchar_t> FetchUrl(const std::string&);

		// Threadpool related
		struct ThreadData {
			ThreadData(NewsFetcher*, const StoryId, const std::size_t, const std::function<void(Story, std::size_t)>*);
			ThreadData& operator=(const ThreadData&) = delete;

			NewsFetcher* fetcher;
			const StoryId storyId;
			const std::size_t index;
			const std::function<void(Story, std::size_t)> *callback;
		};
		static void CALLBACK ThreadCallback(PTP_CALLBACK_INSTANCE, void*, PTP_WORK);
	};
} // namespace hackernewscmd