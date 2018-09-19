// variableSystem.cpp - manage user variables ($variables)

#include "common.h"

#ifdef INFORMATION
There are 5 kinds of variables.
1. User variables beginning wih $(regular and transient which begin with $$)
2. Wildcard variables beginning with _
3. Fact sets beginning with @
4. Function variables beginning with ^
5. System variables beginning with %
#endif
unsigned int userVariableThreadList = 0;
int impliedSet = ALREADY_HANDLED;	// what fact set is involved in operation
int impliedWild = ALREADY_HANDLED;	// what wildcard is involved in operation
char impliedOp = 0;					// for impliedSet, what op is in effect += = 

int wildcardIndex = 0;
char wildcardOriginalText[MAX_WILDCARDS+1][MAX_USERVAR_SIZE+1];  // spot wild cards can be stored
char wildcardCanonicalText[MAX_WILDCARDS+1][MAX_USERVAR_SIZE+1];  // spot wild cards can be stored
unsigned int wildcardPosition[MAX_WILDCARDS+1]; // spot it started and ended in sentence
char wildcardSeparator[2];

//   list of active variables needing saving

WORDP tracedFunctionsList[MAX_TRACED_FUNCTIONS];
unsigned int kernelVariableThreadList = 0;
unsigned int botVariableThreadList = 0;
unsigned int tracedFunctionsIndex;
unsigned int modifiedTraceVal = 0;
bool modifiedTrace = false;

void InitVariableSystem()
{
	*wildcardSeparator = ' ';
	wildcardSeparator[1] = 0;
	kernelVariableThreadList = botVariableThreadList = userVariableThreadList = tracedFunctionsIndex = 0;
}

int GetWildcardID(char* x) // wildcard id is "_10" or "_3"
{
	if (!IsDigit(x[1])) return ILLEGAL_MATCHVARIABLE;
	unsigned int n = x[1] - '0';
	char c = x[2];
	if (IsDigit(c)) n =  (n * 10) + (c - '0');
	return (n > MAX_WILDCARDS) ? ILLEGAL_MATCHVARIABLE : n; 
}

static void CompleteWildcard()
{
	WORDP D = FindWord(wildcardCanonicalText[wildcardIndex]);
	if (D && D->properties & D->internalBits & UPPERCASE_HASH)  // but may not be found if original has plural or such or if uses _
	{
		strcpy(wildcardCanonicalText[wildcardIndex],D->word);
	}

    ++wildcardIndex;
	if (wildcardIndex > MAX_WILDCARDS) wildcardIndex = 0; 
}

void JoinMatch(int start, int end,int index,bool inpattern)
{
	// concatenate the match value
	bool started = false;
	for (int i = start; i <= end; ++i)
	{
		if (i < 1 || i > wordCount) continue;	// ignore off end
		if (unmarked[i]) continue; // ignore words masked
		char* word = wordStarts[i];
		if (started)
		{
			strcat(wildcardOriginalText[index], wildcardSeparator);
			strcat(wildcardCanonicalText[index], wildcardSeparator);
		}
		else started = true;
		strcat(wildcardOriginalText[index], word);
		if (wordCanonical[i]) strcat(wildcardCanonicalText[index], wordCanonical[i]);
		else strcat(wildcardCanonicalText[index], word);
	}
	if (strstr(wildcardCanonicalText[index], "unknown-word")) strcpy(wildcardCanonicalText[index], "unknown-word"); // if any are unknown, the composite is unknown
	// force a particular case? do we know the word?
	int lookup = STANDARD_LOOKUP;
	if (uppercaseFind > 0) 
		lookup = UPPERCASE_LOOKUP;
	else if (uppercaseFind == 0) lookup = LOWERCASE_LOOKUP;
	WORDP D = FindWord(wildcardCanonicalText[index], 0, lookup);
	if (D) strcpy(wildcardCanonicalText[index], D->word); // but may not be found if original has plural or such or if uses _
	D = FindWord(wildcardOriginalText[index], 0, lookup);
	if (D) strcpy(wildcardOriginalText[index], D->word); // but may not be found if original has plural or such or if uses _

	if (trace & TRACE_OUTPUT && !inpattern && CheckTopicTrace()) Log(STDTRACELOG, (char*)"_%d=%s/%s ", index, wildcardOriginalText[index], wildcardCanonicalText[index]);
}

void SetWildCard(int start, int end, bool inpattern)
{
	if (end < start) end = start;				// matched within a token
	if (end > wordCount)
	{
		if (start != end) end = wordCount; // for start==end we allow being off end, eg _>
		else start = end = wordCount;
	}
	while (unmarked[start]) ++start; // skip over unmarked words at start
	while (unmarked[end]) --end; // skip over unmarked words at end
    wildcardPosition[wildcardIndex] = start | (end << 16);
    *wildcardOriginalText[wildcardIndex] = 0;
    *wildcardCanonicalText[wildcardIndex] = 0;
    if (start == 0 || wordCount == 0 || (end == 0 && start != 1) ) // null match, like _{ .. }
	{
		++wildcardIndex;
		if (wildcardIndex > MAX_WILDCARDS) wildcardIndex = 0; 
	}
	else // did match
	{
		JoinMatch(start, end,wildcardIndex,inpattern);
		CompleteWildcard();
	}
}

void SetWildCardGiven(int start, int end, bool inpattern, int index)
{
	if (end < start) end = start;				// matched within a token
	if (end > wordCount)
	{
		if (start != end) end = wordCount + 1; // for start==end we allow being off end, eg _>
		else start = end = wordCount + 1;
	}
    *wildcardOriginalText[index] = 0;
    *wildcardCanonicalText[index] = 0;
 	while (unmarked[start]) ++start; // skip over unmarked words at start
	while (unmarked[end]) --end; // skip over unmarked words at end
	wildcardPosition[index] = start | (end << 16);
    if (start == 0 || wordCount == 0 || (end == 0 && start != 1) ) // null match, like _{ .. }
	{
	}
	else JoinMatch(start, end, index, inpattern); // did match
 }

void SetWildCardNull()
{
	SetWildCardGivenValue((char*)"", (char*)"",0,0, wildcardIndex);
	CompleteWildcard();
}

void SetWildCardGivenValue(char* original, char* canonical,int start, int end, int index)
{
	if (end < start) end = start;				// matched within a token
	if (end > wordCount && start != end) end = wordCount; // for start==end we allow being off end, eg _>
    wildcardPosition[index] = start | (end << 16);
    *wildcardOriginalText[index] = 0;
    *wildcardCanonicalText[index] = 0;
    if (start == 0 || wordCount == 0 || (end == 0 && start != 1) ) // null match, like _{ .. }
	{
	}
	else JoinMatch(start, end, index, false); // did match
}

void SetWildCardIndexStart(int index)
{
	 wildcardIndex = index;
}

void SetWildCard(char* value, char* canonicalValue,const char* index,unsigned int position)
{
	// adjust values to assign
	if (!value) value = "";
	if (!canonicalValue) canonicalValue = "";
    if (strlen(value) > MAX_USERVAR_SIZE) 
	{
		value[MAX_USERVAR_SIZE] = 0;
		ReportBug((char*)"Too long matchvariable original value %s",value)
	}
     if (strlen(canonicalValue) > MAX_USERVAR_SIZE) 
	{
		canonicalValue[MAX_USERVAR_SIZE] = 0;
		ReportBug((char*)"Too long matchvariable canonical value %s",value)
	}
	while (value[0] == ' ') ++value; 
    while (canonicalValue && canonicalValue[0] == ' ') ++canonicalValue;

	// store the values
	if (index) wildcardIndex = GetWildcardID((char*)index); 
    strcpy(wildcardOriginalText[wildcardIndex],value);
    strcpy(wildcardCanonicalText[wildcardIndex],(canonicalValue) ? canonicalValue : value);
    wildcardPosition[wildcardIndex] = position | (position << 16); 
  
	CompleteWildcard();
}

char* GetwildcardText(unsigned int i, bool canon)
{
	if (i > MAX_WILDCARDS) return "";
    return canon ? wildcardCanonicalText[i] : wildcardOriginalText[i];
}

char* GetUserVariable(const char* word,bool nojson,bool fortrace)
{
	int len = 0;
	const char* separator = (nojson) ? (const char*) NULL : strchr(word,'.');
	const char* bracket = (nojson) ? (const char*) NULL : strchr(word,'[');
	if (!separator && bracket) separator = bracket;
	if (bracket && bracket < separator) separator = bracket;	// this happens first
	if (separator) len = separator - word;
	bool localvar = strstr(word,"$_") != NULL; // local var anywhere in the chain?
    char* item = NULL;
	char* answer;	
	char path[MAX_WORD_SIZE];
	*path = 0;
    
	WORDP D = FindWord((char*) word,len,LOWERCASE_LOOKUP);
    if (!D) { goto NULLVALUE; }	//   no such variable

	item = D->w.userValue;
  	if (localvar) // is $_local variable whose value we get
	{
		if (!item)  goto NULLVALUE;
		if (*item == LCLVARDATA_PREFIX && item[1] == LCLVARDATA_PREFIX) // should be true
		{
			item += 2; // skip `` marker
		}
	}
	else if (!item) goto NULLVALUE; // null value normal

	if (separator) // json object or array follows this
	{
		if (fortrace) localvar = false;	// dont allocate memory
		strcpy(path,item);
LOOPDEEPER:
		FACT* factvalue = NULL;
		if (IsDigitWord(item, AMERICAN_NUMBERS) && (!strnicmp(separator,".subject",8) || !strnicmp(separator,".verb",5) ||!strnicmp(separator,".object",7)) ) // fact id?
		{
			int val = atoi(item);
			factvalue = Index2Fact(val);
			if (!factvalue) goto NULLVALUE;
		}
		else D = FindWord(item); // the basic item
		if (!D) goto NULLVALUE;
		if (*separator == '.' && strncmp(item,"jo-",3) && !factvalue) goto NULLVALUE; // cannot be dotted
		if (*separator == '[' && strncmp(item,"ja-",3)) goto NULLVALUE; // cannot be indexed

		// is there more later
		char* separator1 =  (char*)strchr(separator+1,'.');	// more dot like $x.y.z?
		char* bracket1 = (char*)strchr(separator+1,'[');	// more [ like $x[y][z]?
		if (!bracket1) bracket1 = (char*)strchr(separator+1,']'); // is there a closer as final
		if (bracket1 && !separator1) separator1 = bracket1;
		if (bracket1 && bracket1 < separator1) separator1 = bracket1;

		// get the label for this current separator
		char* label = (char*)separator + 1;
		if (separator1)
		{
			len = (separator1 - label);
			if (*(separator1-1) == ']') --len;	// prior close of array ref
		}
		else len = 0;
		WORDP key;
		if (factvalue) 
		{
			if (separator[1] == 's' || separator[1] == 'S') label = "subject"; 
			else if (separator[1] == 'v' || separator[1] == 'V') label = "verb";
			else label = "object";
		}
		else if (*separator == '.') // it is a key
		{
			key = FindWord(label,len,PRIMARY_CASE_ALLOWED); // case sensitive find
			if (!key) goto NULLVALUE; // dont recognize such a name
			label = key->word;
			if (separator[1] == '$') // indirection
			{
				label = GetUserVariable(key->word); // key is a variable name , go get it to use real key
				key = FindWord(label);
				if (!key) goto NULLVALUE;	// cannot find
			}
		}
		else // it is an index - of either array OR object
		{
			if (IsDigit(*label)) 
			{
				char* end = strchr(label,']');
				key = FindWord(label,end-label);
				if (!key) goto NULLVALUE;
				label = key->word;
			}
			else if (*label == '$') // indirect
			{
				char val[MAX_WORD_SIZE];
				strncpy(val,label,len);
				val[len]  = 0;
				label = GetUserVariable(val); // key is a variable name or index, go get it to use real key
				if (IsDigit(*label)) key = FindWord(label);
				if (!IsDigit(*label))  goto NULLVALUE;
			}
			else  goto NULLVALUE; // not a number or indirect
		}
		if (*separator == '.') strcat(path,".");
		else strcat(path,"[");
		strcat(path,label);
		if (*separator != '.') strcat(path,"]");
		if (factvalue) // $x.subject
		{
			if (separator[1] == 's' || separator[1] == 'S') answer = Meaning2Word(factvalue->subject)->word; 
			else if (separator[1] == 'v' || separator[1] == 'V') answer = Meaning2Word(factvalue->verb)->word; 
			else answer = Meaning2Word(factvalue->object)->word;
			separator = separator1;
			if (!separator) goto ANSWER;
			item = answer;
			goto LOOPDEEPER;
		}

		FACT* F = GetSubjectNondeadHead(D);
		MEANING verb = MakeMeaning(key);
		while (F)
		{
			if (F->verb == verb) 
			{
				answer = Meaning2Word(F->object)->word;
				if (!strcmp(answer,"null")) 
				{
					item = "``";
					return item + 2; // null value for locals
				}
				// does it continue?
				if (separator1 && *separator1 == ']') ++separator1; // after current key/index there is another
				if (separator1 && *separator1) // after current key/index there is another
				{
					item = answer;
					separator = separator1;
					goto LOOPDEEPER;
				}
				goto ANSWER;
			}
			F = GetSubjectNondeadNext(F);
		}
		goto NULLVALUE;
	} 
	answer = item;
	// OLD NO LONGER VALID? if item is in fact & there are problems   return (*item == '&') ? (item + 1) : item; //   value is quoted or not

ANSWER:
	if (localvar) 
	{
		char* limit;
		char* ans = InfiniteStack(limit,"GetUserVariable"); // has complete
		strcpy(ans,"``");
		strcpy(ans+2,answer);
		CompleteBindStack();
		if (*path && trace & TRACE_VARIABLE && !fortrace) Log(STDTRACELOG,"(%s->%s)",path,ans);
		return ans + 2;
	}
	if (*path && trace & TRACE_VARIABLE && !fortrace) Log(STDTRACELOG,"(%s->%s)",path,answer);
	return answer;

NULLVALUE:
	if (localvar)
	{
		item = "``";
		return item + 2; // null value for locals
	}
	return "";
 }

void ClearUserVariableSetFlags()
{
	unsigned int varthread = userVariableThreadList;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		RemoveInternalFlag(D, VAR_CHANGED);
	}
}

void ShowChangedVariables()
{
	unsigned int varthread = userVariableThreadList;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		
		char* value = D->w.userValue;
		if (value && *value) Log(1,(char*)"%s = %s\r\n",D->word,value);
		else Log(1,(char*)"%s = null\r\n",D->word);
	}
}

void PrepareVariableChange(WORDP D,char* word,bool init)
{
	if (D->word[1] == '_') // tmp var
	{
		if (init) D->w.userValue = NULL;
	}
	else if (!(D->internalBits & VAR_CHANGED))	// not changed already this volley
    {
		char* data = AllocateHeap(NULL, 2, 4); // word  aligned
		((unsigned int*)data)[0] = userVariableThreadList;
		((unsigned int*)data)[1] = Word2Index(D);
		userVariableThreadList = Heap2Index(data);

		if (init) D->w.userValue = NULL; 
		D->internalBits |= VAR_CHANGED; // bypasses even locked preexisting variables
	}
}

void SetUserVariable(const char* var, char* word, bool assignment)
{
	char varname[MAX_WORD_SIZE];
	MakeLowerCopy(varname,(char*)var);
    WORDP D = StoreWord(varname);				// find or create the var.
	if (!D) return; // ran out of memory

#ifndef DISCARDTESTING
	CheckAssignment(varname,word);
#endif

	// adjust value
	if (word) // has a nonnull value?
	{
		if (!*word || !stricmp(word,(char*)"null") ) word = NULL; // really is null
		else //   some value 
		{
			bool purelocal = (D->word[1] == LOCALVAR_PREFIX);
			if (purelocal) word = AllocateStack(word,0,true); // trying to save on permanent space
			else word = AllocateHeap(word,0,1,false,purelocal); // we may be restoring old value which doesnt need allocation
			if (!word) return;
		}
	}

	PrepareVariableChange(D,word,true);
	if (planning && !documentMode) // handle undoable assignment (cannot use text sharing as done in document mode)
	{
		if (D->w.userValue == NULL) SpecialFact(MakeMeaning(D),(MEANING)1,0);
		else SpecialFact(MakeMeaning(D),(MEANING) (D->w.userValue - heapBase ),0);
	}
	D->w.userValue = word; 

	// tokencontrol changes are noticed by the engine
	if (!stricmp(var,(char*)"$cs_token")) 
	{
		int64 val = 0;
		if (word && *word) ReadInt64(word,val);
		else 
		{
			val = (DO_INTERJECTION_SPLITTING|DO_SUBSTITUTE_SYSTEM|DO_NUMBER_MERGE|DO_PROPERNAME_MERGE|DO_SPELLCHECK);
			if (!stricmp(language,"english")) val |= DO_PARSE;
		}
		tokenControl = val;
	}
	// cs_float changes are noticed by the engine
	if (!stricmp(var, (char*)"$cs_fullfloat")) 
		fullfloat = (word && *word) ? true : false;
	// cs_numbers changes are noticed by the engine (india, french, other)
	else if (!stricmp(var, (char*)"$cs_numbers"))
	{
		if (!word) numberStyle = AMERICAN_NUMBERS;
		else if (!stricmp(word, "indian")) numberStyle = INDIAN_NUMBERS;
		else if (!stricmp(word, "french")) numberStyle = FRENCH_NUMBERS;
		else numberStyle = AMERICAN_NUMBERS;

		if (numberStyle == FRENCH_NUMBERS)
		{
			numberComma = '.';
			numberPeriod = ',';
		}
		else
		{
			numberComma = ',';
			numberPeriod = '.';
		}
	}
	// trace
	else if (!stricmp(var,(char*)"$cs_trace")) 
	{
		int64 val = 0;
		if (word && *word) ReadInt64(word,val);
		trace = (unsigned int)val;
		if (assignment) // remember script changed it
		{
			modifiedTraceVal = trace;
			modifiedTrace = true;
		}
	}	
	// responsecontrol changes are noticed by the engine
	else if (!stricmp(var,(char*)"$cs_response")) 
	{
		int64 val = 0;
		if (word && *word) ReadInt64(word,val);
		else val = ALL_RESPONSES;
		responseControl = (unsigned int)val;
	}	
	// cs_botid changes are noticed by the engine
	else if (!stricmp(var,(char*)"$cs_botid")) 
	{
		int64 val = 0;
		if (word && *word) ReadInt64(word,val);
		myBot = (uint64)val;
	}	
	// wildcardseparator changes are noticed by the engine
	else if (!stricmp(var,(char*)"$cs_wildcardSeparator")) 
	{
		if (*word == '\\') *wildcardSeparator = word[2];
		else *wildcardSeparator = (*word == '"') ? word[1] : *word; // 1st char in string if need be
	}	
	if (trace && D->internalBits & MACRO_TRACE) 
	{
		char pattern[110];
		char label[MAX_LABEL_SIZE];
		GetPattern(currentRule,label,pattern,100);  // go to output
		Log(ECHOSTDTRACELOG,"%s -> %s at %s.%d.%d %s %s\r\n",D->word,word, GetTopicName(currentTopicID),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),label,pattern);
	}
}

void Add2UserVariable(char* var, char* moreValue,char* op,char* originalArg)
{
	// get original value
	char  minusflag = *op;
	char* oldValue;
    if (*var == '_') oldValue = GetwildcardText(GetWildcardID(var),true); // onto a wildcard
	else if (*var == USERVAR_PREFIX) oldValue = GetUserVariable(var); // onto user variable
	else if (*var == '^') oldValue = callArgumentList[atoi(var+1)+fnVarBase]; // onto function argument
	else return; // illegal

	// get augment value
	if (*moreValue == '_') moreValue = GetwildcardText(GetWildcardID(moreValue),true); 
	else if (*moreValue == USERVAR_PREFIX) moreValue = GetUserVariable(moreValue); 
	else if (*moreValue == '^') moreValue = callArgumentList[atoi(moreValue+1)+fnVarBase];

	// perform numeric op
	bool floating = false;
	if (strchr(oldValue,'.') || strchr(moreValue,'.') || *op == '/' ) floating = true; 
	char result[MAX_WORD_SIZE];

    if (floating)
    {
        double newval = Convert2Float(oldValue);
		double more = Convert2Float(moreValue);
        if (minusflag == '-') newval -= more;
        else if (minusflag == '*') newval *= more;
        else if (minusflag == '/') {
			if (more == 0) return; // cannot divide by 0
        	newval /= more;
        }
		else if (minusflag == '%') 
		{
			if (more == 0) return;
			int64 ivalue = (int64) newval;
			int64 morval = (int64) more;
			newval = (double) (ivalue % morval);
		}
        else newval += more;
        WriteFloat(result,newval);
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACELOG,(char*)" %s   ",result);
    }
    else
    {
		int64 newval;
		ReadInt64(oldValue,newval);
		int64 more;
		ReadInt64(moreValue,more);
        if (minusflag == '-') newval -= more;
        else if (minusflag == '*') newval *= more;
        else if (minusflag == '/') 
		{
			if (more == 0)  return; // cannot divide by 0
			newval /= more;
		}
        else if (minusflag == '%') 
		{
			if (more == 0) return;
			newval %= more;
		}
        else if (minusflag == '|') 
		{
			newval |= more;
			if (op[1] == '^') newval ^= more;
		}
        else if (minusflag == '&') newval &= more;
        else if (minusflag == '^') newval ^= more;
        else if (minusflag == '<') newval <<= more;
		else if (minusflag == '>') newval >>= more;
		else newval += more;
		char tracex[MAX_WORD_SIZE];
#ifdef WIN32
		sprintf(result,(char*)"%I64d",newval); 
#else
		sprintf(result,(char*)"%lld",newval); 
#endif        
  		if (trace & TRACE_OUTPUT && CheckTopicTrace()) 
		{
			sprintf(tracex,"0x%016llx",newval);
			Log(STDTRACELOG,(char*)" %s/%s   ",result,tracex);
		}
	}

	// store result back
	if (*var == '_')  
		SetWildCard(result,result,var,0); 
	else if (*var == USERVAR_PREFIX) 
	{
		char* dot = strchr(var,'.');
		if (!dot) SetUserVariable(var,result,true);
		else JSONVariableAssign(var,result);// json object insert
	}
	else if (*var == '^') strcpy(callArgumentList[atoi(var+1)+fnVarBase],result); 
}

static void SetBotVars(unsigned int varthread)
{
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		D->w.userValue = Index2Heap(cell[2]);
	}
}

void ReestablishBotVariables() // refresh bot variables in case user overwrote them
{
	SetBotVars(botVariableThreadList);
	SetBotVars(kernelVariableThreadList);
}

void ClearBotVariables()
{
	kernelVariableThreadList = 0;
	botVariableThreadList = 0;
}

void NoteBotVariables() // system defined variables
{
	unsigned int varthread = userVariableThreadList;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		if (D->word[1] != LOCALVAR_PREFIX && D->word[1] != TRANSIENTVAR_PREFIX) // not a transient var
		{
			if (!strnicmp(D->word,"$cs_",4)) continue; // dont force these, they are for user
			int* data = (int*)AllocateHeap(NULL, 3, 4);
			data[0] = botVariableThreadList;
			data[1] = Word2Index(D);
			data[2] = Heap2Index(D->w.userValue);
			botVariableThreadList = Heap2Index((char*)data);
		}
		RemoveInternalFlag(D,VAR_CHANGED);
	}
	userVariableThreadList = 0;
}

void MigrateUserVariables()
{
	unsigned int varthread = userVariableThreadList; // does not include $_ locals
	userVariableThreadList = 0;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];

		WORDP D = Index2Word(cell[1]);
		D->w.userValue = AllocateStack(D->w.userValue, 0);
		D->word = AllocateStack(D->word, 0); // reallocate name

		// recreate thread list
		unsigned int* data = (unsigned int*)AllocateStack(NULL, 8, false, true); // allocate list
		data[0] = userVariableThreadList;
		data[1] = cell[1];
		userVariableThreadList = Stack2Index((char*)data);
	}
}

void RecoverUserVariables()
{
	unsigned int varthread = userVariableThreadList;
	userVariableThreadList = 0;
	while (varthread)
	{ 
		unsigned int* cell = (unsigned int*)Index2Stack(varthread);
		varthread = cell[0];

		WORDP D = Index2Word(cell[1]); // 0 based
        D->w.userValue = AllocateHeap(D->w.userValue, 0);
		D->word = AllocateHeap(D->word, 0);

		unsigned int* data = (unsigned int*)AllocateHeap(NULL, 2, 4); // allocate list
		data[0] = userVariableThreadList;
		data[1] = cell[1];
		userVariableThreadList = Heap2Index((char*)data);
	}
}

void ClearUserVariables(char* above) 
{
	unsigned int varthread = userVariableThreadList;
	unsigned int* prevcell = 0;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		if (!above) // removing ALL variables
		{	
			D->w.userValue = NULL;
			RemoveInternalFlag(D,VAR_CHANGED);
		}
		else  if (D->w.userValue < above) // heap spaces runs DOWN, so this passes more recent entries into here
		{
			if (prevcell) prevcell[0] = varthread;  // previous needs to point to next
			else userVariableThreadList = varthread;  // potential new start of list
		}
		else prevcell = cell;  // keeping this one, so remember it
	}
	if (!above) userVariableThreadList = 0;
}

// This is the comparison function for the sort operation.
static int compareVariables(const void *var1, const void *var2)
{
	return strcmp((*(WORDP *)var1)->word, (*(WORDP *)var2)->word);
}

static void ListVariables(char* header,unsigned int varthread)
{
	char* value;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		value = D->w.userValue;
		if (value && *value)  Log(STDTRACELOG, (char*)"  %s variable: %s = %s\r\n", header,D->word, value);
	}
}

void DumpUserVariables()
{
	char* value;
	ListVariables("preboot", kernelVariableThreadList);
	ListVariables("boot", botVariableThreadList);

	// count entries
	int counter = 0; 
	unsigned int varthread = userVariableThreadList;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		++counter;
	}

	// Show the user variables in alphabetically sorted order.
	WORDP *arySortVariablesHelper = (WORDP*) AllocateStack(NULL,((counter) ? counter : 1) * sizeof(WORDP));

	// Load the array.
	varthread = userVariableThreadList;
	int i = 0;
	while (varthread)
	{
		unsigned int* cell = (unsigned int*)Index2Heap(varthread);
		varthread = cell[0];
		WORDP D = Index2Word(cell[1]);
		arySortVariablesHelper[i++] = D;
	}

	// Sort it.
	qsort(arySortVariablesHelper, counter, sizeof(WORDP), compareVariables);

	// Display the variables in sorted order.
	for (i = 0; i < counter; ++i)
	{
		WORDP D = arySortVariablesHelper[i];
		value = D->w.userValue;
		if (value && *value)
		{
			if (!stricmp(D->word, "$cs_token"))
			{
				Log(STDTRACELOG, "  variable: $cs_token decoded = ");
				int64 val;
				ReadInt64(value, val);
				DumpTokenControls(val);
				Log(STDTRACELOG, "\r\n");
			}
			else Log(STDTRACELOG, "  variable: %s = %s\r\n", D->word, value);
		}
	}
	ReleaseStack((char*) arySortVariablesHelper); // short term
}

char* PerformAssignment(char* word,char* ptr,char* buffer,FunctionResult &result,bool nojson)
{// assign to and from  $var, _var, ^var, @set, and %sysvar
    char op[MAX_WORD_SIZE];
	currentFact = NULL;					// No assignment can start with a fact lying around
	int oldImpliedSet = impliedSet;		// in case nested calls happen
	int oldImpliedWild = impliedWild;	// in case nested calls happen
    int assignFromWild = ALREADY_HANDLED;
	result = NOPROBLEM_BIT;
	
	if (*word == '^' && word[1] == '^' && IsDigit(word[2])) // indirect function variable assign
	{
		char* value = callArgumentList[atoi(word+2) + fnVarBase];
		if (*value == LCLVARDATA_PREFIX && value[1] == LCLVARDATA_PREFIX)//  not allowed to write indirect to caller arg
		{
			result = FAILRULE_BIT;
			return ptr;
		}
		strcpy(word,value); // change over to indirect to assign onto
		strcpy(word,GetUserVariable(word)); // now indirect thru him if we can
		if (!*word)
		{
			result = FAILRULE_BIT;
			return ptr;
		}
	}
	else if (*word == '^' && IsDigit(word[1])) // indirect function variable assign
	{
		char* value = callArgumentList[atoi(word+1) + fnVarBase];
		if (*value == LCLVARDATA_PREFIX && value[1] == LCLVARDATA_PREFIX)//  not allowed to write indirect to caller arg
		{
			result = FAILRULE_BIT;
			return ptr;
		}
		strcpy(word,value); // change over to assign onto caller var
	}

	impliedSet = ALREADY_HANDLED;
	impliedWild = ALREADY_HANDLED;
	if (*word == '@')
	{
		impliedSet = GetSetID(word);
		if (impliedSet == ILLEGAL_FACTSET)
		{
			result = FAILRULE_BIT;
			return ptr;
		}
		char* at = word + 1;
		while (*++at && IsDigit(*at)){;} // find end
		if (*at) // not allowed to assign onto annotated factset
		{
			result = FAILRULE_BIT;
			return ptr;
		}
	}
	else if (*word == '_')  
	{
		impliedWild = GetWildcardID(word);	// if a wildcard save location
		if (impliedWild == ILLEGAL_MATCHVARIABLE) 
		{
			result = FAILRULE_BIT;
			return ptr;
		}
	}
	int setToImply = impliedSet; // what he originally requested
	int setToWild = impliedWild; // what he originally requested
	bool otherassign = (*word != '@') && (*word != '_');

	// Get assignment operator
    ptr = ReadCompiledWord(ptr,op); // assignment operator = += -= /= *= %= ^= |= &=
	impliedOp = *op;
	if (*op == '=' && impliedSet >= 0) SET_FACTSET_COUNT(impliedSet,0); // force to be empty for @0 = ^first(...)
	char originalWord1[MAX_WORD_SIZE];
	ReadCompiledWord(ptr,originalWord1);

	// get the from value
	assignFromWild =  (*ptr == '_' && IsDigit(ptr[1])) ? GetWildcardID(ptr)  : -1;
	if (*word == '_' && *ptr == '\'' && ptr[1] == '_' && IsDigit(ptr[2]))
	{
		assignFromWild = GetWildcardID(ptr + 1); // allow quoted assign across
		ptr = ReadCompiledWord(ptr + 1, buffer);
	}
	else if (assignFromWild >= 0 && *word == '_') ptr = ReadCompiledWord(ptr,buffer); // assigning from wild to wild. Just copy across
	else
	{
		ptr = GetCommandArg(ptr,buffer,result,OUTPUT_NOCOMMANUMBER|ASSIGNMENT); // need to see null assigned -- store raw numbers, not with commas, lest relations break
		if (*buffer == '#') // substitute a constant? user type-in :set command for example
		{
			uint64 n = FindValueByName(buffer+1);
			if (!n) n = FindSystemValueByName(buffer+1);
			if (n) 
			{
#ifdef WIN32
				sprintf(buffer,(char*)"%I64d",(long long int) n); 
#else
				sprintf(buffer,(char*)"%lld",(long long int) n); 
#endif	
			}
		}
		if (result & ENDCODES) goto exit;
		// impliedwild will be used up when spreading a fact by assigned. impliedset will get used up if assigning to a factset.
		// assigning to a var is simple
		// A fact was created but not used up by retrieving some field of it. Convert to a reference to fact.
		// if we already did a conversion into a set or wildcard, dont do it here. Otherwise do it now into those or $uservars.
		// DO NOT CHANGE TO add: if settowild != IMPLIED_WILD. that introduces a bug assigning to $vars.

		// if he original requested to assign to and the assignment has been done, then we dont need to do anything
		if ((setToImply != impliedSet && setToImply != ALREADY_HANDLED) || (setToWild != impliedWild  && setToWild != ALREADY_HANDLED) ) currentFact = NULL; // used up
		// A fact was created but not used up by retrieving some field of it. Convert to a reference to fact. -- eg $$f = createfact()
		else if (currentFact && setToImply == impliedSet && setToWild == impliedWild && (setToWild != ALREADY_HANDLED || setToImply != ALREADY_HANDLED || otherassign)) sprintf(buffer,(char*)"%d",currentFactIndex());
	}
	// normally null is empty string, but for assignment allow the word, which means null as in removal
   	if (!stricmp(buffer,(char*)"null") && (*word != USERVAR_PREFIX || !strchr(word,'.'))) *buffer = 0; 

	//   now sort out who we are assigning into and if its arithmetic or simple assign
	if (*word == '@')
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"%s(%s) %s %s => ",word,GetUserVariable(word),op,originalWord1);
		if (impliedSet == ALREADY_HANDLED){;}
		else if (!*buffer && *op == '=') // null assign to set as a whole
		{
			if (*originalWord1 == '^') {;} // presume its a factset function and it has done its job - @0 = next(fact @1fact)
			else SET_FACTSET_COUNT(impliedSet,0);
		}
		else if (*buffer == '@') // set to set operator
		{
			FACT* F;
			int rightSet = GetSetID(buffer);
			if (rightSet == ILLEGAL_FACTSET) 
			{
				result = FAILRULE_BIT;
				impliedOp = 0;
				goto exit;
			}
			unsigned int rightCount =  FACTSET_COUNT(rightSet);
			unsigned int impliedCount =  FACTSET_COUNT(impliedSet); 

			if (*op == '+') while (rightCount) AddFact(impliedSet,factSet[rightSet][rightCount--]); // add set to set preserving order
			else if (*op == '-') // remove from set
			{
				for (unsigned int i = 1; i <= rightCount; ++i) factSet[rightSet][i]->flags |= MARKED_FACT; // mark right facts
				for (unsigned int i = 1; i <= impliedCount; ++i) // erase from the left side
				{ 
					F = factSet[impliedSet][i];
					if (F->flags & MARKED_FACT)
					{
						memmove(&factSet[impliedSet][i],&factSet[impliedSet][i+1], (impliedCount - i)* sizeof(FACT*));
						--impliedCount; // new end count
						--i; // redo loop at this point
					}
				}
				for (unsigned int i = 1; i <= rightCount; ++i) factSet[rightSet][i]->flags ^= MARKED_FACT; // erase marks
				SET_FACTSET_COUNT(impliedSet,impliedCount);
			}
			else if (*op == '=') memmove(&factSet[impliedSet][0],&factSet[rightSet][0], (rightCount+1) * sizeof(FACT*)); // assigned from set
			else result = FAILRULE_BIT;
		}
		else if (IsDigit(*buffer) || !*buffer) // fact index (or null fact) to set operators 
		{
			int index;
			ReadInt(buffer,index);
			FACT* F = Index2Fact(index);
			unsigned int impliedCount =  FACTSET_COUNT(impliedSet); 
			if (*op == '+' && op[1] == '=' && !stricmp(originalWord1,(char*)"^query") ) {;} // add to set done directly @2 += ^query()
			else if (*op == '+') AddFact(impliedSet,F); // add to set
			else if (*op == '-') // remove from set
			{
				if (F) F->flags |= MARKED_FACT;
				for (unsigned int i = 1; i <= impliedCount; ++i) // erase from the left side
				{ 
					FACT* G = factSet[impliedSet][i];
					if ((G && G->flags & MARKED_FACT) || (!G && !F))
					{
						memmove(&factSet[impliedSet][i],&factSet[impliedSet][i+1], (impliedCount - i)* sizeof(FACT*) );
						--impliedCount;
						--i;
					}
				}
				SET_FACTSET_COUNT(impliedSet,impliedCount);
				F->flags ^= MARKED_FACT;
			}
			else if (*op == '=') // assign to set (cant do this with null because it erases the set)
			{
				SET_FACTSET_COUNT(impliedSet,0);
				AddFact(impliedSet,F);
			}
			else result = FAILRULE_BIT;
		}
		if (impliedSet != ALREADY_HANDLED)
		{
			factSetNext[impliedSet] = 0; // all changes requires a reset of next ptr
			impliedSet = ALREADY_HANDLED;
		}
	}
	else if (IsArithmeticOperator(op))  
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) 
		{
			if (*op == '=') Log(STDTRACETABLOG,(char*)"%s %s %s(%s) => ",word,op,originalWord1,GetUserVariable(originalWord1));
			else Log(STDTRACETABLOG,(char*)"%s(%s) %s %s(%s) => ",word,GetUserVariable(word),op,originalWord1,GetUserVariable(originalWord1));
		}
		if (*word == '^') result = FAILRULE_BIT;	// not allowed to increment locally at present OR json array ref...
		else Add2UserVariable(word,buffer,op,originalWord1);
	}
	else if (*word == '_') //   assign to wild card
	{
		if (impliedWild != ALREADY_HANDLED) // no one has actually done the assignnment yet
		{
			if (assignFromWild >= 0) // full tranfer of data
			{
				SetWildCard(wildcardOriginalText[assignFromWild],wildcardCanonicalText[assignFromWild],word,0); 
				wildcardPosition[GetWildcardID(word)] =  wildcardPosition[assignFromWild];
				strcpy(buffer, wildcardOriginalText[assignFromWild]); // for tracing
			}
			else SetWildCard(buffer,buffer,word,0); 
		}
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"%s = %s(%s)\r\n",word,originalWord1,buffer);
	}
	else if (*word == USERVAR_PREFIX) 
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"%s = %s(%s)\r\n",word,originalWord1,buffer);
		char* dot = strchr(word,'.');
		if (!dot) dot = strstr(word, "[]"); // array assign?
		if (!dot || nojson) SetUserVariable(word,buffer,true);
		else result = JSONVariableAssign(word,buffer);// json object insert
	}
	else if (*word == '\'' && word[1] == USERVAR_PREFIX) 
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"%s = %s(%s)\r\n",word,originalWord1,buffer);
		SetUserVariable(word+1,buffer,true); // '$xx = value  -- like passed thru as argument
	}
	else if (*word == SYSVAR_PREFIX) 
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"%s = %s(%s)\r\n",word,originalWord1,buffer);
		SystemVariable(word,buffer);
	}
	else if (*word == '^') // overwrite function arg
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"%s = %s\r\n",word,buffer);
		callArgumentList[atoi(word+1)+fnVarBase] = AllocateStack(buffer);
	}
	else // cannot touch a  word, or number
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"illegal assignment to %s\r\n",word);
		result = FAILRULE_BIT;
		goto exit;
	}

	//  followup arithmetic operators?
	while (ptr && IsArithmeticOperator(ptr))
	{
		ptr = ReadCompiledWord(ptr,op);
		ReadCompiledWord(ptr,originalWord1);
		ptr = GetCommandArg(ptr,buffer,result,ASSIGNMENT); 
		if (!stricmp(buffer,word)) 
		{
			result = FAILRULE_BIT;
			Log(STDTRACELOG,(char*)"variable assign %s has itself as a term\r\n",word);
		}
		if (result & ENDCODES) goto exit; // failed next value
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"    %s(%s) %s %s(%s) =>",word,GetUserVariable(word),op,originalWord1,buffer);
		Add2UserVariable(word,buffer,op,originalWord1);
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACELOG,(char*)"\r\n");
	}

	// debug
	if (trace & TRACE_OUTPUT && CheckTopicTrace())
	{
		logUpdated = false;
		if (*word == '@') 
		{
			int set = GetSetID(word);
			int count = FACTSET_COUNT(set);
			FACT* F = factSet[set][count];
			unsigned int id = Fact2Index(F);
			char fact[MAX_WORD_SIZE];
			WriteFact(F,false,fact,false,false);
			Log(STDUSERLOG,(char*)"last value @%d[%d] is %d %s",set,count,id,fact ); // show last item in set
		}
	}
	
exit:
	currentFact = NULL; // any assignment uses up current fact by definition
	*buffer = 0;
	impliedSet = oldImpliedSet;
	impliedWild = oldImpliedWild;
	impliedOp = 0;
	return ptr;
}
