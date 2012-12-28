/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_EMBED_LOG_H
#define MOZ_EMBED_LOG_H

#define FORCE_PR_LOG
#define PR_LOGGING 1

#include "prlog.h"

#include <stdio.h>

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetEmbedCommonLog(const char* aModule);

#ifdef LOG_COMPONENT
#define LOGF(FMT, ARG...) PR_LOG(GetEmbedCommonLog(LOG_COMPONENT), PR_LOG_DEBUG, (LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGT(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLiteTrace"), PR_LOG_DEBUG, (LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGW(FMT, ARG...) PR_LOG(GetEmbedCommonLog(LOG_COMPONENT), PR_LOG_WARNING, ("W:" LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGE(FMT, ARG...) PR_LOG(GetEmbedCommonLog(LOG_COMPONENT), PR_LOG_ERROR, ("E:" LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGNI(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedNonImpl"), PR_LOG_ALWAYS, ("NON_IMPL:" LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#else
#define LOGF(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLite"), PR_LOG_DEBUG, ("EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGT(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLiteTrace"), PR_LOG_DEBUG, ("EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGW(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLite"), PR_LOG_WARNING, ("W: EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGE(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLite"), PR_LOG_ERROR, ("E: EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGNI(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedNonImpl"), PR_LOG_ALWAYS, ("NON_IMPL: EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#endif

#define LOGC(CUSTOMNAME, FMT, ARG...) PR_LOG(GetEmbedCommonLog(CUSTOMNAME), PR_LOG_DEBUG, (CUSTOMNAME "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))

#else

#define LOGF(...) do {} while (0)
#define LOGT(...) do {} while (0)
#define LOGW(...) do {} while (0)
#define LOGE(...) do {} while (0)
#define LOGC(...) do {} while (0)

#endif

#endif // MOZ_EMBED_LOG_H
