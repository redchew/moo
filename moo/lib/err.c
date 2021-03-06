/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "moo-prv.h"

/* BEGIN: GENERATED WITH generr.moo */

static moo_ooch_t errstr_0[] = {'n','o',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_1[] = {'g','e','n','e','r','i','c',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_2[] = {'n','o','t',' ','i','m','p','l','e','m','e','n','t','e','d','\0'};
static moo_ooch_t errstr_3[] = {'s','u','b','s','y','s','t','e','m',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_4[] = {'i','n','t','e','r','n','a','l',' ','e','r','r','o','r',' ','t','h','a','t',' ','s','h','o','u','l','d',' ','n','e','v','e','r',' ','h','a','v','e',' ','h','a','p','p','e','n','e','d','\0'};
static moo_ooch_t errstr_5[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','s','y','s','t','e','m',' ','m','e','m','o','r','y','\0'};
static moo_ooch_t errstr_6[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','o','b','j','e','c','t',' ','m','e','m','o','r','y','\0'};
static moo_ooch_t errstr_7[] = {'i','n','v','a','l','i','d',' ','c','l','a','s','s','/','t','y','p','e','\0'};
static moo_ooch_t errstr_8[] = {'i','n','v','a','l','i','d',' ','p','a','r','a','m','e','t','e','r',' ','o','r',' ','a','r','g','u','m','e','n','t','\0'};
static moo_ooch_t errstr_9[] = {'d','a','t','a',' ','n','o','t',' ','f','o','u','n','d','\0'};
static moo_ooch_t errstr_10[] = {'e','x','i','s','t','i','n','g','/','d','u','p','l','i','c','a','t','e',' ','d','a','t','a','\0'};
static moo_ooch_t errstr_11[] = {'b','u','s','y','\0'};
static moo_ooch_t errstr_12[] = {'a','c','c','e','s','s',' ','d','e','n','i','e','d','\0'};
static moo_ooch_t errstr_13[] = {'o','p','e','r','a','t','i','o','n',' ','n','o','t',' ','p','e','r','m','i','t','t','e','d','\0'};
static moo_ooch_t errstr_14[] = {'n','o','t',' ',' ','d','i','r','e','c','t','o','r','y','\0'};
static moo_ooch_t errstr_15[] = {'i','n','t','e','r','r','u','p','t','e','d','\0'};
static moo_ooch_t errstr_16[] = {'p','i','p','e',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_17[] = {'r','e','s','o','u','r','c','e',' ','t','e','m','p','o','r','a','r','i','l','y',' ','u','n','a','v','a','i','l','a','b','l','e','\0'};
static moo_ooch_t errstr_18[] = {'b','a','d',' ','s','y','s','t','e','m',' ','h','a','n','d','l','e','\0'};
static moo_ooch_t errstr_19[] = {'*','*','*',' ','u','n','d','e','f','i','n','e','d',' ','e','r','r','o','r',' ','*','*','*','\0'};
static moo_ooch_t errstr_20[] = {'m','e','s','s','a','g','e',' ','r','e','c','e','i','v','e','r',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_21[] = {'m','e','s','s','a','g','e',' ','s','e','n','d','i','n','g',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_22[] = {'w','r','o','n','g',' ','n','u','m','b','e','r',' ','o','f',' ','a','r','g','u','m','e','n','t','s','\0'};
static moo_ooch_t errstr_23[] = {'r','a','n','g','e',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_24[] = {'b','y','t','e','-','c','o','d','e',' ','f','u','l','l','\0'};
static moo_ooch_t errstr_25[] = {'d','i','c','t','i','o','n','a','r','y',' ','f','u','l','l','\0'};
static moo_ooch_t errstr_26[] = {'p','r','o','c','e','s','s','o','r',' ','f','u','l','l','\0'};
static moo_ooch_t errstr_27[] = {'t','o','o',' ','m','a','n','y',' ','s','e','m','a','p','h','o','r','e','s','\0'};
static moo_ooch_t errstr_28[] = {'*','*','*',' ','u','n','d','e','f','i','n','e','d',' ','e','r','r','o','r',' ','*','*','*','\0'};
static moo_ooch_t errstr_29[] = {'d','i','v','i','d','e',' ','b','y',' ','z','e','r','o','\0'};
static moo_ooch_t errstr_30[] = {'I','/','O',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_31[] = {'e','n','c','o','d','i','n','g',' ','c','o','n','v','e','r','s','i','o','n',' ','e','r','r','o','r','\0'};
static moo_ooch_t errstr_32[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','d','a','t','a',' ','f','o','r',' ','e','n','c','o','d','i','n','g',' ','c','o','n','v','e','r','s','i','o','n','\0'};
static moo_ooch_t errstr_33[] = {'b','u','f','f','e','r',' ','f','u','l','l','\0'};
static moo_ooch_t* errstr[] =
{
	errstr_0, errstr_1, errstr_2, errstr_3, errstr_4, errstr_5, errstr_6, errstr_7,
	errstr_8, errstr_9, errstr_10, errstr_11, errstr_12, errstr_13, errstr_14, errstr_15,
	errstr_16, errstr_17, errstr_18, errstr_19, errstr_20, errstr_21, errstr_22, errstr_23,
	errstr_24, errstr_25, errstr_26, errstr_27, errstr_28, errstr_29, errstr_30, errstr_31,
	errstr_32, errstr_33 
};

#if defined(MOO_INCLUDE_COMPILER)
static moo_ooch_t synerrstr_0[] = {'n','o',' ','e','r','r','o','r','\0'};
static moo_ooch_t synerrstr_1[] = {'i','l','l','e','g','a','l',' ','c','h','a','r','a','c','t','e','r','\0'};
static moo_ooch_t synerrstr_2[] = {'c','o','m','m','e','n','t',' ','n','o','t',' ','c','l','o','s','e','d','\0'};
static moo_ooch_t synerrstr_3[] = {'s','t','r','i','n','g',' ','n','o','t',' ','c','l','o','s','e','d','\0'};
static moo_ooch_t synerrstr_4[] = {'n','o',' ','c','h','a','r','a','c','t','e','r',' ','a','f','t','e','r',' ','$','\0'};
static moo_ooch_t synerrstr_5[] = {'n','o',' ','v','a','l','i','d',' ','c','h','a','r','a','c','t','e','r',' ','a','f','t','e','r',' ','#','\0'};
static moo_ooch_t synerrstr_6[] = {'n','o',' ','v','a','l','i','d',' ','c','h','a','r','a','c','t','e','r',' ','a','f','t','e','r',' ','#','\\','\0'};
static moo_ooch_t synerrstr_7[] = {'w','r','o','n','g',' ','c','h','a','r','a','c','t','e','r',' ','l','i','t','e','r','a','l','\0'};
static moo_ooch_t synerrstr_8[] = {'c','o','l','o','n',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_9[] = {'s','t','r','i','n','g',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_10[] = {'i','n','v','a','l','i','d',' ','r','a','d','i','x','\0'};
static moo_ooch_t synerrstr_11[] = {'i','n','v','a','l','i','d',' ','i','n','t','e','g','e','r',' ','l','i','t','e','r','a','l','\0'};
static moo_ooch_t synerrstr_12[] = {'i','n','v','a','l','i','d',' ','f','i','x','e','d','-','p','o','i','n','t',' ','d','e','c','i','m','a','l',' ','s','c','a','l','e','\0'};
static moo_ooch_t synerrstr_13[] = {'i','n','v','a','l','i','d',' ','f','i','x','e','d','-','p','o','i','n','t',' ','d','e','c','i','m','a','l',' ','l','i','t','e','r','a','l','\0'};
static moo_ooch_t synerrstr_14[] = {'b','y','t','e',' ','t','o','o',' ','s','m','a','l','l',' ','o','r',' ','t','o','o',' ','l','a','r','g','e','\0'};
static moo_ooch_t synerrstr_15[] = {'w','r','o','n','g',' ','e','r','r','o','r',' ','l','i','t','e','r','a','l','\0'};
static moo_ooch_t synerrstr_16[] = {'w','r','o','n','g',' ','s','m','p','t','r',' ','l','i','t','e','r','a','l','\0'};
static moo_ooch_t synerrstr_17[] = {'{',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_18[] = {'}',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_19[] = {'(',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_20[] = {')',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_21[] = {']',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_22[] = {'.',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_23[] = {',',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_24[] = {'|',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_25[] = {'>',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_26[] = {':','=',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_27[] = {'i','d','e','n','t','i','f','i','e','r',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_28[] = {'i','n','t','e','g','e','r',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_29[] = {'p','r','i','m','i','t','i','v','e',':',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_30[] = {'w','r','o','n','g',' ','d','i','r','e','c','t','i','v','e','\0'};
static moo_ooch_t synerrstr_31[] = {'w','r','o','n','g',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_32[] = {'d','u','p','l','i','c','a','t','e',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_33[] = {'u','n','d','e','f','i','n','e','d',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_34[] = {'c','o','n','t','r','a','d','i','c','t','o','r','y',' ','c','l','a','s','s',' ','d','e','f','i','n','i','t','i','o','n','\0'};
static moo_ooch_t synerrstr_35[] = {'c','l','a','s','s',' ','n','o','t',' ','c','o','n','f','o','r','m','i','n','g',' ','t','o',' ','i','n','t','e','r','f','a','c','e','\0'};
static moo_ooch_t synerrstr_36[] = {'i','n','v','a','l','i','d',' ','n','o','n','-','p','o','i','n','t','e','r',' ','i','n','s','t','a','n','c','e',' ','s','i','z','e','\0'};
static moo_ooch_t synerrstr_37[] = {'p','r','o','h','i','b','i','t','e','d',' ','i','n','h','e','r','i','t','a','n','c','e','\0'};
static moo_ooch_t synerrstr_38[] = {'v','a','r','i','a','b','l','e',' ','d','e','c','l','a','r','a','t','i','o','n',' ','n','o','t',' ','a','l','l','o','w','e','d','\0'};
static moo_ooch_t synerrstr_39[] = {'m','o','d','i','f','i','e','r',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_40[] = {'w','r','o','n','g',' ','m','o','d','i','f','i','e','r','\0'};
static moo_ooch_t synerrstr_41[] = {'d','i','s','a','l','l','o','w','e','d',' ','m','o','d','i','f','i','e','r','\0'};
static moo_ooch_t synerrstr_42[] = {'d','u','p','l','i','c','a','t','e',' ','m','o','d','i','f','i','e','r','\0'};
static moo_ooch_t synerrstr_43[] = {'m','e','t','h','o','d',' ','n','a','m','e',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_44[] = {'d','u','p','l','i','c','a','t','e',' ','m','e','t','h','o','d',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_45[] = {'i','n','v','a','l','i','d',' ','v','a','r','i','a','d','i','c',' ','m','e','t','h','o','d',' ','d','e','f','i','n','i','t','i','o','n','\0'};
static moo_ooch_t synerrstr_46[] = {'v','a','r','i','a','b','l','e',' ','n','a','m','e',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_47[] = {'d','u','p','l','i','c','a','t','e',' ','a','r','g','u','m','e','n','t',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_48[] = {'d','u','p','l','i','c','a','t','e',' ','t','e','m','p','o','r','a','r','y',' ','v','a','r','i','a','b','l','e',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_49[] = {'d','u','p','l','i','c','a','t','e',' ','v','a','r','i','a','b','l','e',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_50[] = {'d','u','p','l','i','c','a','t','e',' ','b','l','o','c','k',' ','a','r','g','u','m','e','n','t',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_51[] = {'u','n','d','e','c','l','a','r','e','d',' ','v','a','r','i','a','b','l','e','\0'};
static moo_ooch_t synerrstr_52[] = {'u','n','u','s','a','b','l','e',' ','v','a','r','i','a','b','l','e',' ','i','n',' ','c','o','m','p','i','l','e','d',' ','c','o','d','e','\0'};
static moo_ooch_t synerrstr_53[] = {'i','n','a','c','c','e','s','s','i','b','l','e',' ','v','a','r','i','a','b','l','e','\0'};
static moo_ooch_t synerrstr_54[] = {'a','m','b','i','g','u','o','u','s',' ','v','a','r','i','a','b','l','e','\0'};
static moo_ooch_t synerrstr_55[] = {'t','o','o',' ','m','a','n','y',' ','i','n','s','t','a','n','c','e','/','c','l','a','s','s',' ','v','a','r','i','a','b','l','e','s','\0'};
static moo_ooch_t synerrstr_56[] = {'i','n','a','c','c','e','s','s','i','b','l','e',' ','s','u','p','e','r','\0'};
static moo_ooch_t synerrstr_57[] = {'w','r','o','n','g',' ','e','x','p','r','e','s','s','i','o','n',' ','p','r','i','m','a','r','y','\0'};
static moo_ooch_t synerrstr_58[] = {'t','o','o',' ','m','a','n','y',' ','t','e','m','p','o','r','a','r','i','e','s','\0'};
static moo_ooch_t synerrstr_59[] = {'t','o','o',' ','m','a','n','y',' ','a','r','g','u','m','e','n','t','s','\0'};
static moo_ooch_t synerrstr_60[] = {'t','o','o',' ','m','a','n','y',' ','b','l','o','c','k',' ','t','e','m','p','o','r','a','r','i','e','s','\0'};
static moo_ooch_t synerrstr_61[] = {'t','o','o',' ','m','a','n','y',' ','b','l','o','c','k',' ','a','r','g','u','m','e','n','t','s','\0'};
static moo_ooch_t synerrstr_62[] = {'a','r','r','a','y',' ','e','x','p','r','e','s','s','i','o','n',' ','t','o','o',' ','l','a','r','g','e','\0'};
static moo_ooch_t synerrstr_63[] = {'i','n','s','t','r','u','c','t','i','o','n',' ','d','a','t','a',' ','t','o','o',' ','l','a','r','g','e','\0'};
static moo_ooch_t synerrstr_64[] = {'w','r','o','n','g',' ','p','r','i','m','i','t','i','v','e',' ','f','u','n','c','t','i','o','n',' ','n','u','m','b','e','r','\0'};
static moo_ooch_t synerrstr_65[] = {'w','r','o','n','g',' ','p','r','i','m','i','t','i','v','e',' ','f','u','n','c','t','i','o','n',' ','i','d','e','n','t','i','f','i','e','r','\0'};
static moo_ooch_t synerrstr_66[] = {'w','r','o','n','g',' ','p','r','i','m','i','t','i','v','e',' ','f','u','n','c','t','i','o','n',' ','a','r','g','u','m','e','n','t',' ','d','e','f','i','n','i','t','i','o','n','\0'};
static moo_ooch_t synerrstr_67[] = {'w','r','o','n','g',' ','p','r','i','m','i','t','i','v','e',' ','v','a','l','u','e',' ','i','d','e','n','t','i','f','i','e','r','\0'};
static moo_ooch_t synerrstr_68[] = {'p','r','i','m','i','t','i','v','e',' ','v','a','l','u','e',' ','l','o','a','d',' ','f','r','o','m',' ','m','o','d','u','l','e',' ','n','o','t',' ','a','l','l','o','w','e','d','\0'};
static moo_ooch_t synerrstr_69[] = {'f','a','i','l','e','d',' ','t','o',' ','i','m','p','o','r','t',' ','m','o','d','u','l','e','\0'};
static moo_ooch_t synerrstr_70[] = {'#','i','n','c','l','u','d','e',' ','e','r','r','o','r','\0'};
static moo_ooch_t synerrstr_71[] = {'w','r','o','n','g',' ','p','r','a','g','m','a',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_72[] = {'w','r','o','n','g',' ','n','a','m','e','s','p','a','c','e',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_73[] = {'w','r','o','n','g',' ','p','o','o','l','d','i','c',' ','i','m','p','o','r','t',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_74[] = {'d','u','p','l','i','c','a','t','e',' ','p','o','o','l','d','i','c',' ','i','m','p','o','r','t',' ','n','a','m','e','\0'};
static moo_ooch_t synerrstr_75[] = {'l','i','t','e','r','a','l',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_76[] = {'b','r','e','a','k',' ','o','r',' ','c','o','n','t','i','n','u','e',' ','n','o','t',' ','w','i','t','h','i','n',' ','a',' ','l','o','o','p','\0'};
static moo_ooch_t synerrstr_77[] = {'b','r','e','a','k',' ','o','r',' ','c','o','n','t','i','n','u','e',' ','w','i','t','h','i','n',' ','a',' ','b','l','o','c','k','\0'};
static moo_ooch_t synerrstr_78[] = {'w','h','i','l','e',' ','e','x','p','e','c','t','e','d','\0'};
static moo_ooch_t synerrstr_79[] = {'i','n','v','a','l','i','d',' ','g','o','t','o',' ','t','a','r','g','e','t','\0'};
static moo_ooch_t synerrstr_80[] = {'l','a','b','e','l',' ','a','t',' ','e','n','d','\0'};
static moo_ooch_t* synerrstr[] =
{
	synerrstr_0, synerrstr_1, synerrstr_2, synerrstr_3, synerrstr_4, synerrstr_5, synerrstr_6, synerrstr_7,
	synerrstr_8, synerrstr_9, synerrstr_10, synerrstr_11, synerrstr_12, synerrstr_13, synerrstr_14, synerrstr_15,
	synerrstr_16, synerrstr_17, synerrstr_18, synerrstr_19, synerrstr_20, synerrstr_21, synerrstr_22, synerrstr_23,
	synerrstr_24, synerrstr_25, synerrstr_26, synerrstr_27, synerrstr_28, synerrstr_29, synerrstr_30, synerrstr_31,
	synerrstr_32, synerrstr_33, synerrstr_34, synerrstr_35, synerrstr_36, synerrstr_37, synerrstr_38, synerrstr_39,
	synerrstr_40, synerrstr_41, synerrstr_42, synerrstr_43, synerrstr_44, synerrstr_45, synerrstr_46, synerrstr_47,
	synerrstr_48, synerrstr_49, synerrstr_50, synerrstr_51, synerrstr_52, synerrstr_53, synerrstr_54, synerrstr_55,
	synerrstr_56, synerrstr_57, synerrstr_58, synerrstr_59, synerrstr_60, synerrstr_61, synerrstr_62, synerrstr_63,
	synerrstr_64, synerrstr_65, synerrstr_66, synerrstr_67, synerrstr_68, synerrstr_69, synerrstr_70, synerrstr_71,
	synerrstr_72, synerrstr_73, synerrstr_74, synerrstr_75, synerrstr_76, synerrstr_77, synerrstr_78, synerrstr_79,
	synerrstr_80 
};
#endif
/* END: GENERATED WITH generr.moo */

/* -------------------------------------------------------------------------- 
 * ERROR NUMBER TO STRING CONVERSION
 * -------------------------------------------------------------------------- */
const moo_ooch_t* moo_errnum_to_errstr (moo_errnum_t errnum)
{
	static moo_ooch_t e_unknown[] = {'u','n','k','n','o','w','n',' ','e','r','r','o','r','\0'};
	return (errnum >= 0 && errnum < MOO_COUNTOF(errstr))? errstr[errnum]: e_unknown;
}

#if defined(MOO_INCLUDE_COMPILER)
static const moo_ooch_t* synerr_to_errstr (moo_synerrnum_t errnum)
{
	static moo_ooch_t e_unknown[] = {'u','n','k','n','o','w','n',' ','e','r','r','o','r','\0'};
	return (errnum >= 0 && errnum < MOO_COUNTOF(synerrstr))? synerrstr[errnum]: e_unknown;
}
#endif

/* -------------------------------------------------------------------------- 
 * ERROR NUMBER/MESSAGE HANDLING
 * -------------------------------------------------------------------------- */
const moo_ooch_t* moo_geterrstr (moo_t* moo)
{
	return moo_errnum_to_errstr(moo->errnum);
}

const moo_ooch_t* moo_geterrmsg (moo_t* moo)
{
	if (moo->errmsg.len <= 0) return moo_errnum_to_errstr(moo->errnum);
	return moo->errmsg.buf;
}

void moo_geterrinf (moo_t* moo, moo_errinf_t* info)
{
	info->num = moo_geterrnum(moo);
	moo_copy_oocstr (info->msg, MOO_COUNTOF(info->msg), moo_geterrmsg(moo));
}

const moo_ooch_t* moo_backuperrmsg (moo_t* moo)
{
	moo_copy_oocstr (moo->errmsg.tmpbuf.ooch, MOO_COUNTOF(moo->errmsg.tmpbuf.ooch), moo_geterrmsg(moo));
	return moo->errmsg.tmpbuf.ooch;
}

void moo_seterrnum (moo_t* moo, moo_errnum_t errnum)
{
	if (moo->shuterr) return;
	moo->errnum = errnum; 
	moo->errmsg.len = 0; 
}


static int err_bchars (moo_fmtout_t* fmtout, const moo_bch_t* ptr, moo_oow_t len)
{
	moo_t* moo = (moo_t*)fmtout->ctx;
	moo_oow_t max;

	max = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len - 1;

#if defined(MOO_OOCH_IS_UCH)
	if (max <= 0) return 1;
	moo_conv_bchars_to_uchars_with_cmgr (ptr, &len, &moo->errmsg.buf[moo->errmsg.len], &max, moo_getcmgr(moo), 1);
	moo->errmsg.len += max;
#else
	if (len > max) len = max;
	if (len <= 0) return 1;
	MOO_MEMCPY (&moo->errmsg.buf[moo->errmsg.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->errmsg.len += len;
#endif

	moo->errmsg.buf[moo->errmsg.len] = '\0';

	return 1; /* success */
}

static int err_uchars (moo_fmtout_t* fmtout, const moo_uch_t* ptr, moo_oow_t len)
{
	moo_t* moo = (moo_t*)fmtout->ctx;
	moo_oow_t max;

	max = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len - 1;

#if defined(MOO_OOCH_IS_UCH)
	if (len > max) len = max;
	if (len <= 0) return 1;
	MOO_MEMCPY (&moo->errmsg.buf[moo->errmsg.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->errmsg.len += len;
#else
	if (max <= 0) return 1;
	moo_conv_uchars_to_bchars_with_cmgr (ptr, &len, &moo->errmsg.buf[moo->errmsg.len], &max, moo_getcmgr(moo));
	moo->errmsg.len += max;
#endif
	moo->errmsg.buf[moo->errmsg.len] = '\0';
	return 1; /* success */
}

void moo_seterrbfmt (moo_t* moo, moo_errnum_t errnum, const moo_bch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_t fo;

	if (moo->shuterr) return;
	moo->errmsg.len = 0;

	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.mmgr = moo_getmmgr(moo);
	fo.putbchars = err_bchars;
	fo.putuchars = err_uchars;
	fo.putobj = moo_fmt_object_;
	fo.ctx = moo;

	va_start (ap, fmt);
	moo_bfmt_outv (&fo, fmt, ap);
	va_end (ap);

	moo->errnum = errnum;
}

void moo_seterrufmt (moo_t* moo, moo_errnum_t errnum, const moo_uch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_t fo;

	if (moo->shuterr) return;
	moo->errmsg.len = 0;

	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.mmgr = moo_getmmgr(moo);
	fo.putbchars = err_bchars;
	fo.putuchars = err_uchars;
	fo.putobj = moo_fmt_object_;
	fo.ctx = moo;

	va_start (ap, fmt);
	moo_ufmt_outv (&fo, fmt, ap);
	va_end (ap);

	moo->errnum = errnum;
}


void moo_seterrbfmtv (moo_t* moo, moo_errnum_t errnum, const moo_bch_t* fmt, va_list ap)
{
	moo_fmtout_t fo;

	if (moo->shuterr) return;

	moo->errmsg.len = 0;

	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.mmgr = moo_getmmgr(moo);
	fo.putbchars = err_bchars;
	fo.putuchars = err_uchars;
	fo.putobj = moo_fmt_object_;
	fo.ctx = moo;

	moo_bfmt_outv (&fo, fmt, ap);
	moo->errnum = errnum;
}

void moo_seterrufmtv (moo_t* moo, moo_errnum_t errnum, const moo_uch_t* fmt, va_list ap)
{
	moo_fmtout_t fo;

	if (moo->shuterr) return;

	moo->errmsg.len = 0;

	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.mmgr = moo_getmmgr(moo);
	fo.putbchars = err_bchars;
	fo.putuchars = err_uchars;
	fo.putobj = moo_fmt_object_;
	fo.ctx = moo;

	moo_ufmt_outv (&fo, fmt, ap);
	moo->errnum = errnum;
}


void moo_seterrwithsyserr (moo_t* moo, int syserr_type, int syserr_code)
{
	moo_errnum_t errnum;

	if (moo->shuterr) return;

	if (moo->vmprim.syserrstrb)
	{
		errnum = moo->vmprim.syserrstrb(moo, syserr_type, syserr_code, moo->errmsg.tmpbuf.bch, MOO_COUNTOF(moo->errmsg.tmpbuf.bch));
		moo_seterrbfmt (moo, errnum, "%hs", moo->errmsg.tmpbuf.bch);
	}
	else
	{
		MOO_ASSERT (moo, moo->vmprim.syserrstru != MOO_NULL);
		errnum = moo->vmprim.syserrstru(moo, syserr_type, syserr_code, moo->errmsg.tmpbuf.uch, MOO_COUNTOF(moo->errmsg.tmpbuf.uch));
		moo_seterrbfmt (moo, errnum, "%ls", moo->errmsg.tmpbuf.uch);
	}
}

void moo_seterrbfmtwithsyserr (moo_t* moo, int syserr_type, int syserr_code, const moo_bch_t* fmt, ...)
{
	moo_errnum_t errnum;
	moo_oow_t ucslen, bcslen;
	va_list ap;

	if (moo->shuterr) return;
	
	if (moo->vmprim.syserrstrb)
	{
		errnum = moo->vmprim.syserrstrb(moo, syserr_type, syserr_code, moo->errmsg.tmpbuf.bch, MOO_COUNTOF(moo->errmsg.tmpbuf.bch));
		
		va_start (ap, fmt);
		moo_seterrbfmtv (moo, errnum, fmt, ap);
		va_end (ap);

		if (MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len >= 5)
		{
			moo->errmsg.buf[moo->errmsg.len++] = ' ';
			moo->errmsg.buf[moo->errmsg.len++] = '-';
			moo->errmsg.buf[moo->errmsg.len++] = ' ';

		#if defined(MOO_OOCH_IS_BCH)
			moo->errmsg.len += moo_copy_bcstr(&moo->errmsg.buf[moo->errmsg.len], MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len, moo->errmsg.tmpbuf.bch);
		#else
			ucslen = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len;
			moo_convbtoucstr (moo, moo->errmsg.tmpbuf.bch, &bcslen, &moo->errmsg.buf[moo->errmsg.len], &ucslen);
			moo->errmsg.len += ucslen;
		#endif
		}
	}
	else
	{
		MOO_ASSERT (moo, moo->vmprim.syserrstru != MOO_NULL);
		errnum = moo->vmprim.syserrstru(moo, syserr_type, syserr_code, moo->errmsg.tmpbuf.uch, MOO_COUNTOF(moo->errmsg.tmpbuf.uch));

		va_start (ap, fmt);
		moo_seterrbfmtv (moo, errnum, fmt, ap);
		va_end (ap);

		if (MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len >= 5)
		{
			moo->errmsg.buf[moo->errmsg.len++] = ' ';
			moo->errmsg.buf[moo->errmsg.len++] = '-';
			moo->errmsg.buf[moo->errmsg.len++] = ' ';

		#if defined(MOO_OOCH_IS_BCH)
			bcslen = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len;
			moo_convutobcstr (moo, moo->errmsg.tmpbuf.uch, &ucslen, &moo->errmsg.buf[moo->errmsg.len], &bcslen);
			moo->errmsg.len += bcslen;
		#else
			moo->errmsg.len += moo_copy_ucstr(&moo->errmsg.buf[moo->errmsg.len], MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len, moo->errmsg.tmpbuf.uch);
		#endif
		}
	}
}

void moo_seterrufmtwithsyserr (moo_t* moo, int syserr_type, int syserr_code, const moo_uch_t* fmt, ...)
{
	moo_errnum_t errnum;
	moo_oow_t ucslen, bcslen;
	va_list ap;

	if (moo->shuterr) return;
	
	if (moo->vmprim.syserrstrb)
	{
		errnum = moo->vmprim.syserrstrb(moo, syserr_type, syserr_code, moo->errmsg.tmpbuf.bch, MOO_COUNTOF(moo->errmsg.tmpbuf.bch));

		va_start (ap, fmt);
		moo_seterrufmtv (moo, errnum, fmt, ap);
		va_end (ap);

		if (MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len >= 5)
		{
			moo->errmsg.buf[moo->errmsg.len++] = ' ';
			moo->errmsg.buf[moo->errmsg.len++] = '-';
			moo->errmsg.buf[moo->errmsg.len++] = ' ';

		#if defined(MOO_OOCH_IS_BCH)
			moo->errmsg.len += moo_copy_bcstr(&moo->errmsg.buf[moo->errmsg.len], MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len, moo->errmsg.tmpbuf.bch);
		#else
			ucslen = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len;
			moo_convbtoucstr (moo, moo->errmsg.tmpbuf.bch, &bcslen, &moo->errmsg.buf[moo->errmsg.len], &ucslen);
			moo->errmsg.len += ucslen;
		#endif
		}
	}
	else
	{
		MOO_ASSERT (moo, moo->vmprim.syserrstru != MOO_NULL);
		errnum = moo->vmprim.syserrstru(moo, syserr_type, syserr_code, moo->errmsg.tmpbuf.uch, MOO_COUNTOF(moo->errmsg.tmpbuf.uch));

		va_start (ap, fmt);
		moo_seterrufmtv (moo, errnum, fmt, ap);
		va_end (ap);

		if (MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len >= 5)
		{
			moo->errmsg.buf[moo->errmsg.len++] = ' ';
			moo->errmsg.buf[moo->errmsg.len++] = '-';
			moo->errmsg.buf[moo->errmsg.len++] = ' ';

		#if defined(MOO_OOCH_IS_BCH)
			bcslen = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len;
			moo_convutobcstr (moo, moo->errmsg.tmpbuf.uch, &ucslen, &moo->errmsg.buf[moo->errmsg.len], &bcslen);
			moo->errmsg.len += bcslen;
		#else
			moo->errmsg.len += moo_copy_ucstr(&moo->errmsg.buf[moo->errmsg.len], MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len, moo->errmsg.tmpbuf.uch);
		#endif
		}
	}
}

#if defined(MOO_INCLUDE_COMPILER)

void moo_setsynerrbfmt (moo_t* moo, moo_synerrnum_t num, const moo_ioloc_t* loc, const moo_oocs_t* tgt, const moo_bch_t* msgfmt, ...)
{
	static moo_bch_t syntax_error[] = "syntax error - ";

	if (moo->shuterr) return;

	if (msgfmt) 
	{
		va_list ap;
		int i, selen;

		va_start (ap, msgfmt);
		moo_seterrbfmtv (moo, MOO_ESYNERR, msgfmt, ap);
		va_end (ap);

		selen = MOO_COUNTOF(syntax_error) - 1;
		MOO_MEMMOVE (&moo->errmsg.buf[selen], &moo->errmsg.buf[0], MOO_SIZEOF(moo->errmsg.buf[0]) * (MOO_COUNTOF(moo->errmsg.buf) - selen));
		for (i = 0; i < selen; i++) moo->errmsg.buf[i] = syntax_error[i];
		moo->errmsg.buf[MOO_COUNTOF(moo->errmsg.buf) - 1] = '\0';
	}
	else 
	{
		moo_seterrbfmt (moo, MOO_ESYNERR, "%hs%js", syntax_error, synerr_to_errstr(num));
	}
	moo->c->synerr.num = num;

	/* The SCO compiler complains of this ternary operation saying:
	 *    error: operands have incompatible types: op ":" 
	 * it seems to complain of type mismatch between *loc and
	 * moo->c->tok.loc due to 'const' prefixed to loc. */
	/*moo->c->synerr.loc = loc? *loc: moo->c->tok.loc;*/
	if (loc)
	{
		moo->c->synerr.loc = *loc;
	}
	else
	{
		moo->c->synerr.loc = moo->c->tok.loc;
	}
	
	if (tgt)
	{
		if (tgt->len >= MOO_COUNTOF(moo->c->synerr_tgtbuf) && 
		    moo_copyoocharstosbuf(moo, tgt->ptr, tgt->len, MOO_SBUF_ID_SYNERR) >= 0)
		{
			moo->c->synerr.tgt.ptr = moo->sbuf[MOO_SBUF_ID_SYNERR].ptr;
			moo->c->synerr.tgt.len = moo->sbuf[MOO_SBUF_ID_SYNERR].len;
		}
		else
		{
			moo->c->synerr.tgt.ptr = moo->c->synerr_tgtbuf;
			moo->c->synerr.tgt.len = (tgt->len < MOO_COUNTOF(moo->c->synerr_tgtbuf))? tgt->len: (MOO_COUNTOF(moo->c->synerr_tgtbuf) - 1);
			moo->c->synerr.tgt.ptr[moo->c->synerr.tgt.len] = '\0';
			moo_copy_oochars (moo->c->synerr.tgt.ptr, tgt->ptr, moo->c->synerr.tgt.len);
		}
	}
	else 
	{
		moo->c->synerr.tgt.ptr = MOO_NULL;
		moo->c->synerr.tgt.len = 0;
	}
}

void moo_setsynerrufmt (moo_t* moo, moo_synerrnum_t num, const moo_ioloc_t* loc, const moo_oocs_t* tgt, const moo_uch_t* msgfmt, ...)
{
	static moo_bch_t syntax_error[] = "syntax error - ";

	if (moo->shuterr) return;

	if (msgfmt) 
	{
		va_list ap;
		int i, selen;

		va_start (ap, msgfmt);
		moo_seterrufmtv (moo, MOO_ESYNERR, msgfmt, ap);
		va_end (ap);

		selen = MOO_COUNTOF(syntax_error) - 1;
		MOO_MEMMOVE (&moo->errmsg.buf[selen], &moo->errmsg.buf[0], MOO_SIZEOF(moo->errmsg.buf[0]) * (MOO_COUNTOF(moo->errmsg.buf) - selen));
		for (i = 0; i < selen; i++) moo->errmsg.buf[i] = syntax_error[i];
		moo->errmsg.buf[MOO_COUNTOF(moo->errmsg.buf) - 1] = '\0';
	}
	else 
	{
		moo_seterrbfmt (moo, MOO_ESYNERR, "%hs%js", syntax_error, synerr_to_errstr(num));
	}
	moo->c->synerr.num = num;

	/* The SCO compiler complains of this ternary operation saying:
	 *    error: operands have incompatible types: op ":" 
	 * it seems to complain of type mismatch between *loc and
	 * moo->c->tok.loc due to 'const' prefixed to loc. */
	/*moo->c->synerr.loc = loc? *loc: moo->c->tok.loc;*/
	if (loc)
	{
		moo->c->synerr.loc = *loc;
	}
	else
	{
		moo->c->synerr.loc = moo->c->tok.loc;
	}

	if (tgt)
	{
		if (tgt->len >= MOO_COUNTOF(moo->c->synerr_tgtbuf) && 
		    moo_copyoocharstosbuf(moo, tgt->ptr, tgt->len, MOO_SBUF_ID_SYNERR) >= 0)
		{
			moo->c->synerr.tgt.ptr = moo->sbuf[MOO_SBUF_ID_SYNERR].ptr;
			moo->c->synerr.tgt.len = moo->sbuf[MOO_SBUF_ID_SYNERR].len;
		}
		else
		{
			moo->c->synerr.tgt.ptr = moo->c->synerr_tgtbuf;
			moo->c->synerr.tgt.len = (tgt->len < MOO_COUNTOF(moo->c->synerr_tgtbuf))? tgt->len: (MOO_COUNTOF(moo->c->synerr_tgtbuf) - 1);
			moo->c->synerr.tgt.ptr[moo->c->synerr.tgt.len] = '\0';
			moo_copy_oochars (moo->c->synerr.tgt.ptr, tgt->ptr, moo->c->synerr.tgt.len);
		}
	}
	else 
	{
		moo->c->synerr.tgt.ptr = MOO_NULL;
		moo->c->synerr.tgt.len = 0;
	}
}

void moo_setsynerr (moo_t* moo, moo_synerrnum_t num, const moo_ioloc_t* loc, const moo_oocs_t* tgt)
{
	moo_setsynerrbfmt (moo, num, loc, tgt, MOO_NULL);
}

void moo_getsynerr (moo_t* moo, moo_synerr_t* synerr)
{
	MOO_ASSERT (moo, moo->c != MOO_NULL);
	if (synerr) *synerr = moo->c->synerr;
}
#endif
