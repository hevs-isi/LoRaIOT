/*************************************************************************//**
 * \file gloabl.h
 * \brief board include selection
 *
 * \author Marc Pignat
 * \copyright Copyright (c) 2019 Marc Pignat
 *
 **************************************************************************//*
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

struct global_t
{
	u8_t lora_TokenReq;
};

#ifdef __cplusplus
extern "C"
{
#endif

extern struct global_t global;

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_H */
