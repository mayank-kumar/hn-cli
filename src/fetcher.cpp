/**
 * @file fetcher.cpp
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


#include "fetcher.h"
#include <future>
#include <memory>
#include <stdexcept>
#include "rapidjson/document.h"


namespace hackernewscmd {
	const std::string NewsFetcher::kBaseUrl = "https://hacker-news.firebaseio.com/v0";
	const std::string NewsFetcher::kTopStories = "/topstories.json";

	NewsFetcher::NewsFetcher() :
		mInternetHandle(NULL),
		mThreadpool(NULL),
		mThreadpoolCallbackEnvironment(NULL) {}

	NewsFetcher::~NewsFetcher() {
		if (mInternetHandle != NULL && InternetCloseHandle(mInternetHandle) != TRUE) {
			throw std::runtime_error("Could not close internet handle");
		}
		if (mThreadpool != NULL) {
			CloseThreadpool(mThreadpool);
		}
		if (mThreadpoolCallbackEnvironment != NULL) {
			DestroyThreadpoolEnvironment(mThreadpoolCallbackEnvironment);
			delete mThreadpoolCallbackEnvironment;
		}
	}

	std::vector<StoryId> NewsFetcher::FetchTopStoryIds() {
		auto topStoriesJson = FetchUrl(kBaseUrl + kTopStories);
		rapidjson::GenericDocument<rapidjson::UTF16<>> document;
		if (document.Parse(&topStoriesJson[0]).HasParseError() || !document.IsArray()) {
			throw std::runtime_error("Error while parsing response JSON");
		}
		std::vector<StoryId> topStoryIds;
		topStoryIds.reserve(document.Size());
		for (long long i = 0; i < document.Size(); ++i) {
			if (!document[i].IsNull()) {
				topStoryIds.emplace_back(document[i].GetUint64());
			}
		}
		return topStoryIds;
	}

	void NewsFetcher::FetchStories(const FetchThreadData* ftd) {
		std::unique_ptr<const FetchThreadData> threadData(ftd);
		PTP_CALLBACK_ENVIRON cbe = GetThreadpoolCallbackEnvironment();
		PTP_CLEANUP_GROUP cug = ::CreateThreadpoolCleanupGroup();
		::SetThreadpoolCallbackCleanupGroup(cbe, cug, NULL);
		ThreadData* td = NULL;
		PTP_WORK work = NULL;

		for (const auto& item : threadData->ToBeLoaded) {
			td = new ThreadData(this, item.first, item.second, &threadData->OnFetchComplete, &threadData->OnFetchFailed);
			if ((work = ::CreateThreadpoolWork(ThreadCallback, td, cbe)) == NULL) {
				threadData->OnFetchFailed(item.second);
				continue;
			}
			::SubmitThreadpoolWork(work);
		}
		::CloseThreadpoolCleanupGroupMembers(cug, FALSE, NULL);
	}

	HINTERNET NewsFetcher::GetInternetHandle() {
		if (mInternetHandle == NULL) {
			if (::InternetAttemptConnect(0) != ERROR_SUCCESS) {
				throw std::runtime_error("Couldn't connect to internet");
			}
			if (::InternetCheckConnectionA(kBaseUrl.c_str(), FLAG_ICC_FORCE_CONNECTION, 0) != TRUE) {
				throw std::runtime_error("Couldn't connect to " + kBaseUrl);
			}
			if ((mInternetHandle = ::InternetOpenA("hncmd", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) == NULL) {
				throw std::runtime_error("Couldn't get internet handle");
			}
		}
		return mInternetHandle;
	}

	PTP_CALLBACK_ENVIRON NewsFetcher::GetThreadpoolCallbackEnvironment() {
		if (mThreadpoolCallbackEnvironment == NULL) {
			if (mThreadpool == NULL && (mThreadpool = ::CreateThreadpool(NULL)) == NULL) {
				throw std::runtime_error("Couldn't create thread pool");
			}
			::SetThreadpoolThreadMaximum(mThreadpool, kMaxThreads);
			mThreadpoolCallbackEnvironment = new TP_CALLBACK_ENVIRON;
			::InitializeThreadpoolEnvironment(mThreadpoolCallbackEnvironment);
			::SetThreadpoolCallbackPool(mThreadpoolCallbackEnvironment, mThreadpool);
		}
		return mThreadpoolCallbackEnvironment;
	}

	std::vector<wchar_t> NewsFetcher::FetchUrl(const std::string& url) {
		HINTERNET internetHandle = GetInternetHandle(), resultHandle;
		if ((resultHandle = ::InternetOpenUrlA(internetHandle, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, NULL)) == NULL) {
			throw std::runtime_error("Couldn't fetch " + url);
		}
		std::vector<wchar_t> resultVector;
		unsigned long bytesRead = 0;
		int wBytesRead = 0;
		char buff[4096];
		wchar_t wbuff[4096];
		while (::InternetReadFile(resultHandle, buff, 4096, &bytesRead)
			&& bytesRead != 0
			&& (wBytesRead = ::MultiByteToWideChar(CP_UTF8, 0, buff, bytesRead, wbuff, 4096)) != 0) {
			resultVector.insert(resultVector.end(), wbuff, wbuff + wBytesRead);
		}
		resultVector.push_back('\0');
		::InternetCloseHandle(resultHandle);
		return resultVector;
	}

	NewsFetcher::ThreadData::ThreadData(
		NewsFetcher* nf,
		const StoryId sid,
		const std::size_t idx,
		const std::function<void(Story, std::size_t)>* success,
		const std::function<void(std::size_t)>* failure) :
		fetcher(nf),
		storyId(sid),
		index(idx),
		successCallback(success),
		failureCallback(failure) {}

	void CALLBACK NewsFetcher::ThreadCallback(PTP_CALLBACK_INSTANCE, void *context, PTP_WORK) {
		std::unique_ptr<ThreadData> td(static_cast<ThreadData *>(context));
		auto itemId = std::to_string(td->storyId);

		auto retriesLeft = 3;
		rapidjson::GenericDocument<rapidjson::UTF16<>> document;
		while (retriesLeft--) {
			std::vector<wchar_t> json;
			try {
				json = td->fetcher->FetchUrl(kBaseUrl + "/item/" + itemId + ".json");
			} catch (const std::runtime_error&) {
				continue;
			}
			if (!document.Parse(&json[0]).HasParseError()) {
				break;
			}
			retriesLeft = 0;
		}
		if (retriesLeft == -1) {
			(*td->failureCallback)(td->index);
			return;
		}
		Story story;
		story.id = td->storyId;
		story.title = document[L"title"].GetString();
		story.url = document[L"url"].GetString();
		story.score = document[L"score"].GetUint();
		if (document.HasMember(L"descendants")) {
			story.descendants = document[L"descendants"].GetUint();
		}
		story.time = document[L"time"].GetInt64();
		story.by = document[L"by"].GetString();

		(*td->successCallback)(story, td->index);
	}
} // namespace hackernewscmd