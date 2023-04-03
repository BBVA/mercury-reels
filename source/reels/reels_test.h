/* Mercury-Reels

	Copyright 2023 Banco Bilbao Vizcaya Argentaria, S.A.

	This product includes software developed at

	BBVA (https://www.bbva.com/)

	  Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	  Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#include "reels.h"

#if defined TEST
#ifndef INCLUDED_CATCH2
#define INCLUDED_CATCH2

#include "catch.hpp"

#endif

using namespace reels;

extern char b64chars[];
char *image_block_as_string(uint8_t *p_in);
char *image_block_as_string(ImageBlock &blk);
bool string_as_image_block(ImageBlock &blk, char *p_in);
String python_set_as_string(char *p_char);

#endif
