/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_EMBED_LOG_H
#define MOZ_EMBED_LOG_H

#include <stdio.h>

#define PR_LOGGING 1

#ifdef PR_LOGGING

#ifdef EMBED_LITE_INTERNAL

#define FORCE_PR_LOG
#include "prlog.h"

extern PRLogModuleInfo* GetEmbedCommonLog(const char* aModule);

#ifdef LOG_COMPONENT
#define LOGF(FMT, ARG...) PR_LOG(GetEmbedCommonLog(LOG_COMPONENT), PR_LOG_DEBUG, (LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGT(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLiteTrace"), PR_LOG_DEBUG, (LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGW(FMT, ARG...) PR_LOG(GetEmbedCommonLog(LOG_COMPONENT), PR_LOG_WARNING, ("W:" LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGE(FMT, ARG...) PR_LOG(GetEmbedCommonLog(LOG_COMPONENT), PR_LOG_ERROR, ("E:" LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGNI(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedNonImpl"), PR_LOG_ALWAYS, ("NON_IMPL:" LOG_COMPONENT "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#else  // NON LOG_COMPONENT
#define LOGF(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLite"), PR_LOG_DEBUG, ("EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGT(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLiteTrace"), PR_LOG_DEBUG, ("EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGW(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLite"), PR_LOG_WARNING, ("W: EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGE(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedLite"), PR_LOG_ERROR, ("E: EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#define LOGNI(FMT, ARG...) PR_LOG(GetEmbedCommonLog("EmbedNonImpl"), PR_LOG_ALWAYS, ("NON_IMPL: EmbedLite::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))
#endif // LOG_COMPONENT

#define LOGC(CUSTOMNAME, FMT, ARG...) PR_LOG(GetEmbedCommonLog(CUSTOMNAME), PR_LOG_DEBUG, (CUSTOMNAME "::%s:%d " FMT , __FUNCTION__, __LINE__, ##ARG))

#else // EMBED_LITE_INTERNAL

#ifdef LOG_COMPONENT
#define LOGT(FMT, ARG...) fprintf(stderr, \
     "EmbedLiteExt %s:%s:%d: " FMT "\n", LOG_COMPONENT, __FUNCTION__, __LINE__, ## ARG)
#else // LOG_COMPONENT
#define LOGT(FMT, ARG...) fprintf(stderr, \
     "EmbedLiteExt %s:%d: " FMT "\n", __FUNCTION__, __LINE__, ## ARG)
#endif // LOG_COMPONENT

#endif // EMBED_LITE_INTERNAL

#else // PR_LOGGING

#define LOGF(...) do {} while (0)
#define LOGT(...) do {} while (0)
#define LOGW(...) do {} while (0)
#define LOGE(...) do {} while (0)
#define LOGC(...) do {} while (0)

#endif // PR_LOGGING

#endif // MOZ_EMBED_LOG_H
