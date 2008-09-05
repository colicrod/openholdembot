#include "StdAfx.h"

#include "CFormula.h"

#include "CGlobal.h"
#include "CSymbols.h"
#include "..\CTransform\CTransform.h"

#include "OpenHoldemDoc.h"
#include "grammar.h"

CFormula			*p_formula = NULL;
CRITICAL_SECTION	CFormula::cs_formula;

CFormula::CFormula(void)
{
	__SEH_SET_EXCEPTION_HANDLER

	__SEH_HEADER

	InitializeCriticalSectionAndSpinCount(&cs_formula, 4000);

	ClearFormula();
	_formula_name = "";

	__SEH_LOGFATAL("CFormula::Constructor : \n");}

CFormula::~CFormula(void)
{
	__SEH_HEADER

	DeleteCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::Destructor : \n");
}

void CFormula::ClearFormula()
{
	__SEH_HEADER

	EnterCriticalSection(&cs_formula);

	_formula.dBankroll = _formula.dDefcon = _formula.dRake = _formula.dNit = 0.0;
	_formula.mHandList.RemoveAll();
	_formula.mFunction.RemoveAll();

	_formula_name = "";

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::ClearFormula : \n");
}

void CFormula::SetDefaultBot()
{
	__SEH_HEADER

	SFunction		func;

	EnterCriticalSection(&cs_formula);

	ClearFormula();

	func.dirty = true;

	func.func = "notes"; func.func_text = defaultCSnotes; _formula.mFunction.Add(func);
	func.func = "dll"; func.func_text = ""; _formula.mFunction.Add(func);
	func.func = "f$alli"; func.func_text = defaultCSalli; _formula.mFunction.Add(func);
	func.func = "f$swag"; func.func_text = defaultCSswag; _formula.mFunction.Add(func);
	func.func = "f$srai"; func.func_text = defaultCSsrai; _formula.mFunction.Add(func);
	func.func = "f$rais"; func.func_text = defaultCSrais; _formula.mFunction.Add(func);
	func.func = "f$call"; func.func_text = defaultCScall; _formula.mFunction.Add(func);
	func.func = "f$prefold"; func.func_text = defaultCSprefold; _formula.mFunction.Add(func);
	func.func = "f$delay"; func.func_text = defaultCSdelay; _formula.mFunction.Add(func);
	func.func = "f$chat"; func.func_text = defaultCSchat; _formula.mFunction.Add(func);
	func.func = "f$P"; func.func_text = defaultCSP; _formula.mFunction.Add(func);
	func.func = "f$play"; func.func_text = defaultCSplay; _formula.mFunction.Add(func);
	func.func = "f$test"; func.func_text = defaultCStest; _formula.mFunction.Add(func);
	func.func = "f$debug"; func.func_text = defaultCSdebug; _formula.mFunction.Add(func);
	_formula.dBankroll = defaultdBankroll;
	_formula.dDefcon = defaultdDefcon;
	_formula.dRake = defaultdRake;
	_formula.dNit = defaultdNit;

	// Create UDFs
	func.func = "f$evrais"; func.func_text = defaultCSevrais; _formula.mFunction.Add(func);
	func.func = "f$evcall"; func.func_text = defaultCSevcall; _formula.mFunction.Add(func);	

	_formula_name = "Default";

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::SetDefaultBot : \n");
}

// Reading a part of a formula, which may be spread
// between two files in case of an old style whf / whx formula.
void CFormula::ReadFormulaFile(CArchive& ar, bool ignoreFirstLine)
{
	__SEH_HEADER

	CString		strOneLine = ""; 
	int			content = 0;
	char		funcname[256] = {0};
	int			start = 0, end = 0;
		
	SFunction	func;	
	SHandList	list;		

	EnterCriticalSection(&cs_formula);

	// Ignore first line (date/time)
	if (ignoreFirstLine)
		ar.ReadString(strOneLine);

	// read data in, one line at a time
	strOneLine = "";
	content = FTnone;
	func.func = "";
	while(ar.ReadString(strOneLine)) 
	{ 

		// If this line marks the beginning of a function, then save the previously
		// collected function, and start a new one
		if (strOneLine.Mid(0,2)=="##") 
		{

			// Save the previously collected function
			if (content == FTlist) 
			{
				// Strip the LFCR off the last line (we need to add CRLF for all but the last line)
				list.list_text.TrimRight("\r\n");
				_formula.mHandList.Add(list); 
			}
			else if (content == FTfunc) 
			{
				func.func_text.TrimRight("\r\n");
				_formula.mFunction.Add(func);
			}


			// Get the function name				
			start = strOneLine.Find("##",0);

			// No need to check the result of start,
			// as this code gets executed only,
			// if a line starts with "##"	
			end = strOneLine.Find("##", start+2);

			// Checking for malformed function header
			// without trailing "##"
			if (end == -1) 
			{
				// Trying to continue gracefully.				
				// Skipping is not possible,
				// as this crashes the formula editor.											
				strcpy_s(funcname, 256, strOneLine.GetString()+start+2);
				funcname[strOneLine.GetLength()]='\0';
				CString the_ErrorMessage = "Malformed function header!\nMissing trailing '##'.\n" 
					+ strOneLine + "\n"
					+ "Trying to continue...";
				MessageBox(0, the_ErrorMessage, "Syntax Error", 
					MB_OK | MB_ICONEXCLAMATION);
			}

			else 
			{
				strcpy_s(funcname, 256, strOneLine.GetString()+start+2);
				funcname[end-2]='\0';
			}

			if (strcmp(funcname, "bankroll") == 0) { _formula.dBankroll = 0.0; content = FTbankroll; }
			
			else if (strcmp(funcname, "defcon") == 0) { _formula.dDefcon = 0.0; content = FTdefcon; }
			
			else if (strcmp(funcname, "rake") == 0) { _formula.dRake = 0.0; content = FTrake; }
			
			else if (strcmp(funcname, "nit") == 0) { _formula.dNit = 0.0; content = FTnit; }
			
			else if (memcmp(funcname, "list", 4) == 0) 
			{ 
				content = FTlist;
				list.list = funcname;
				list.list_text = "";
			}

			else 
			{
				content = FTfunc;
				func.func = funcname;
				func.func_text = "";
				func.dirty = true;
			}
		}

		// Get the function content
		else 
		{
			switch (content) 
			{
				 case FTbankroll:
					 if (strOneLine.GetLength())
						 _formula.dBankroll = atof(strOneLine.GetString());
					 break;
				 case FTdefcon:
					 if (strOneLine.GetLength())
						 _formula.dDefcon = atof(strOneLine.GetString());
					 break;
				 case FTrake:
					 if (strOneLine.GetLength())
						 _formula.dRake = atof(strOneLine.GetString());
					 break;
				 case FTnit:
					 if (strOneLine.GetLength())
						 _formula.dNit = atof(strOneLine.GetString());
					 break;
				 case FTlist:
					 list.list_text.Append(strOneLine); list.list_text.Append("\r\n");
					 break;
				 case FTfunc:
					 func.func_text.Append(strOneLine); func.func_text.Append("\r\n");
					 break;
			}
		}	
	}  

	// Add the last list/function to the CArray on EOF, if it was a list being processed
	if (content == FTlist) 
	{
		list.list_text.TrimRight("\r\n");
		_formula.mHandList.Add(list); 
	}
	else if (content == FTfunc) 
	{
		func.func_text.TrimRight("\r\n");
		_formula.mFunction.Add(func);
	}

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::ReadFormulaFile");
}

void CFormula::WriteFormula(CArchive& ar) 
{
	__SEH_HEADER

	CString		s = "";
	int			i = 0, N = (int) _formula.mFunction.GetSize();
	char		nowtime[26] = {0};

	//  First write the standard formula functions...
	//  These are functions and symbols, that
	//	* are essential to control the behaviour 
	//	  of (nearly) every poker bot.
	//	* configure some very important constants.
	//  Removed f$evcall and f$evraise.
	//  Added f$delay and f$chat.
	s.Format("##%s##\r\n\r\n", get_time(nowtime)); ar.WriteString(s);

	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "notes") 
			ar.WriteString("##notes##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n"); 

	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "dll")
			ar.WriteString("##dll##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");

	s.Format("##bankroll##\r\n%f\r\n\r\n", _formula.dBankroll); ar.WriteString(s);
	
	s.Format("##defcon##\r\n%f\r\n\r\n", _formula.dDefcon); ar.WriteString(s);
	
	s.Format("##rake##\r\n%f\r\n\r\n", _formula.dRake); ar.WriteString(s);
	
	s.Format("##nit##\r\n%d\r\n\r\n", (int) _formula.dNit); ar.WriteString(s);
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$alli")
			ar.WriteString("##f$alli##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$swag") 
			ar.WriteString("##f$swag##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$srai") 
			ar.WriteString("##f$srai##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$rais") 
			ar.WriteString("##f$rais##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$call") 
			ar.WriteString("##f$call##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$prefold") 
			ar.WriteString("##f$prefold##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$delay") 
			ar.WriteString("##f$delay##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$chat") 
			ar.WriteString("##f$chat##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$P") 
			ar.WriteString("##f$P##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$play") 
			ar.WriteString("##f$play##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$test") 
			ar.WriteString("##f$test##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
	
	for (i=0; i<N; i++) 
		if (_formula.mFunction[i].func == "f$debug") 
			ar.WriteString("##f$debug##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");

	// handlists
	for (i=0; i<(int) _formula.mHandList.GetSize(); i++) 
		ar.WriteString("##" + _formula.mHandList[i].list + "##\r\n" + _formula.mHandList[i].list_text + "\r\n\r\n");

	// ...then write the user defined functions.
	for (i=0; i<(int) _formula.mFunction.GetSize(); i++) 
	{
		if (_formula.mFunction[i].func != "notes" &&
			_formula.mFunction[i].func != "dll" &&
			_formula.mFunction[i].func != "f$alli" &&
			_formula.mFunction[i].func != "f$swag" &&
			_formula.mFunction[i].func != "f$srai" &&
			_formula.mFunction[i].func != "f$rais" &&
			_formula.mFunction[i].func != "f$call" &&
			_formula.mFunction[i].func != "f$prefold" &&
			_formula.mFunction[i].func != "f$delay" &&
			_formula.mFunction[i].func != "f$chat" &&
			_formula.mFunction[i].func != "f$P" &&
			_formula.mFunction[i].func != "f$play" &&
			_formula.mFunction[i].func != "f$test" &&
			_formula.mFunction[i].func != "f$debug" ) 
		{
			ar.WriteString("##" + _formula.mFunction[i].func + "##\r\n" + _formula.mFunction[i].func_text + "\r\n\r\n");
		}
	}

	__SEH_LOGFATAL("CFormula::WriteFormula :\n"); 
}

void CFormula::CreateHandListMatrices()
{
	__SEH_HEADER

	int			listnum = 0, i = 0, j = 0;
	CString		token = "";

	EnterCriticalSection(&cs_formula);

		for (listnum=0; listnum<MAX_HAND_LISTS; listnum++)
			for (i=0; i<=12; i++)
				for (j=0; j<=12; j++)
					_formula.inlist[listnum][i][j] = false;

		for (i=0; i<(int) _formula.mHandList.GetSize(); i++)
		{
			listnum = atoi(_formula.mHandList[i].list.Mid(4).GetString());
			
			if(listnum>=MAX_HAND_LISTS)
				continue;
			
			ParseHandList(_formula.mHandList[i].list_text, _formula.inlist[listnum]);
		}

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::CreateHandListMatrices :\n");
}

bool CFormula::ParseAllFormula(HWND hwnd)
{
	// returns true for successful parse of all trees, false otherwise
	__SEH_HEADER

	sData			data;

	data.all_parsed = true;
	data.calling_hwnd = hwnd;
	data.pParent = this;

	CUPDialog		dlg_progress(hwnd, &CFormula::ParseLoop, &data, "Please wait", false);

	dlg_progress.DoModal();

	return data.all_parsed;

	__SEH_LOGFATAL("CFormula::ParseAllFormula :\n");
}

void CFormula::CheckForDefaultFormulaEntries()
{
	__SEH_HEADER

	int			N = 0, i = 0;
	bool		addit = false;
	SFunction	func;	
	
	N = (int) _formula.mFunction.GetSize();
	func.func_text = ""; 
	func.dirty = true; 

	EnterCriticalSection(&cs_formula);

		// notes
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="notes") addit = false;
		if (addit==true)  
		{ 
			func.func = "notes"; 
			_formula.mFunction.Add(func); 
		}

		// dll
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="dll") addit = false;
		if (addit==true)  
		{ 
			func.func = "dll"; 
			_formula.mFunction.Add(func); 
		}

		// f$alli
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$alli") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$alli"; 
			_formula.mFunction.Add(func); 
		}

		// f$swag
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$swag") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$swag"; 
			_formula.mFunction.Add(func); 
		}

		// f$srai
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$srai") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$srai"; 
			_formula.mFunction.Add(func); 
		}

		// f$rais
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$rais") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$rais"; 
			_formula.mFunction.Add(func); 
		}

		// f$call
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$call") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$call"; 
			_formula.mFunction.Add(func); 
		}

		// f$prefold
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$prefold") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$prefold"; 
			_formula.mFunction.Add(func); 
		}

		// f$delay
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$delay") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$delay"; 
			_formula.mFunction.Add(func); 
		}

		// f$chat
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$chat") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$chat"; 
			_formula.mFunction.Add(func); 
		}

		// f$P
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$P") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$P"; 
			_formula.mFunction.Add(func); 
		}

		// f$play
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$play") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$play"; 
			_formula.mFunction.Add(func); 
		}

		// f$test
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$test") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$test"; 
			_formula.mFunction.Add(func); 
		}

		// f$debug
		addit = true;
		for (i=0; i<N; i++)  
			if (_formula.mFunction[i].func=="f$debug") addit = false;
		if (addit==true)  
		{ 
			func.func = "f$debug"; 
			_formula.mFunction.Add(func); 
		}

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::CheckForDefaultFormulaEntries");
}

void CFormula::MarkCacheStale()
{
	__SEH_HEADER

	EnterCriticalSection(&cs_formula);

	for (int i=0; i<_formula.mFunction.GetSize(); i++)
        _formula.mFunction[i].fresh = false;

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::MarkCacheStale");
}

bool CFormula::ParseLoop(const CUPDUPDATA* pCUPDUPData)
{
	__SEH_HEADER

	int				i = 0, j = 0, N = 0;
	CString			s = "";
	bool			result = false;
	int				stopchar = 0;
	int				c = 0, linenum = 0, colnum = 0;
	LARGE_INTEGER	bcount = {0}, ecount = {0}, lFrequency = {0};
	double			time_elapsed = 0.;
	sData			*data = (sData*) (pCUPDUPData->GetAppData());

	pCUPDUPData->SetProgress("", 0, false);

	// init timer
	QueryPerformanceCounter(&bcount);
	QueryPerformanceFrequency(&lFrequency);

	N = (int) data->pParent->formula()->mFunction.GetSize();
	for (i=0; i<N; i++)
	{

		// Update progress dialog
		s.Format("Parsing formula set %s : %.0f%%", data->pParent->formula()->mFunction[i].func.GetString(), (double) i / (double) N * 100.0);
		QueryPerformanceCounter(&ecount);
		time_elapsed = ((double) (ecount.LowPart - bcount.LowPart))/((double) lFrequency.LowPart);
		pCUPDUPData->SetProgress(s.GetString(), (int) ((double) i / (double) N * 100.0), time_elapsed>=3.0);

		// Parse it if it is dirty, and not notes, dll or f$debug
		if (data->pParent->formula()->mFunction[i].dirty == true &&
				data->pParent->formula()->mFunction[i].func != "notes" &&
				data->pParent->formula()->mFunction[i].func != "dll" &&
				data->pParent->formula()->mFunction[i].func != "f$debug")
		{
			p_global->parse_symbol_formula = data->pParent->formula();
			p_global->parse_symbol_stop_strs.RemoveAll();

			EnterCriticalSection(&cs_formula);
			result = parse(&data->pParent->formula()->mFunction[i].func_text, &data->pParent->set_formula()->mFunction[i].tpi, &stopchar);
			LeaveCriticalSection(&cs_formula);

			if (!result)
			{
				linenum = colnum = 1;
				for (c=0; c<stopchar; c++)
				{
					if (data->pParent->formula()->mFunction[i].func_text.Mid(c, 1)=="\n")
					{
						linenum++;
						colnum = 1;
					}
					else
					{
						colnum++;
					}
				}
				s.Format("Error in parse of %s\nLine: %d  Character: %d\n\nNear:\n \"%s\"",
						 data->pParent->formula()->mFunction[i].func.GetString(),
						 linenum, colnum,
						 data->pParent->formula()->mFunction[i].func_text.Mid(stopchar, 40).GetString());
				MessageBox(data->calling_hwnd, s, "PARSE ERROR", MB_OK);
				data->all_parsed = false;
			}

			else if (p_global->parse_symbol_stop_strs.GetSize() != 0)
			{
				s.Format("Error in parse of %s\n\nInvalid symbols:\n",
						 data->pParent->formula()->mFunction[i].func.GetString());
				for (j=0; j<p_global->parse_symbol_stop_strs.GetSize(); j++)
				{
					s.Append("   ");
					s.Append(p_global->parse_symbol_stop_strs[j].c_str());
					s.Append("\n");
				}
				MessageBox(data->calling_hwnd, s, "PARSE ERROR", MB_OK);
				data->all_parsed = false;
			}

			else
			{
				EnterCriticalSection(&cs_formula);
				data->pParent->set_formula()->mFunction[i].dirty = false;
				LeaveCriticalSection(&cs_formula);
			}
		}
	}

	pCUPDUPData->SetProgress("", 100, true);

	return true;

	__SEH_LOGFATAL("CFormula::ParseLoop :\n");
}

const int CFormula::CardIdentHelper(const char c)
{
	__SEH_HEADER

	if (c>='2' && c<='9')
		return c - '0' - 2;

	else if (c=='T')
		return 8;

	else if (c=='J')
		return 9;

	else if (c=='Q')
		return 10;

	else if (c=='K')
		return 11;

	else if (c=='A')
		return 12;

	return -1;

	__SEH_LOGFATAL("CFormula::CardIdentHelper :\n");
}

void CFormula::ParseHandList(const CString &list_text, bool inlist[13][13])
{
	__SEH_HEADER

	EnterCriticalSection(&cs_formula);

		for (int i=0; i<=12; i++)
			for (int j=0; j<=12; j++)
				inlist[i][j] = false;

		int	token_card0_rank = 0, token_card1_rank = 0, temp_rank = 0;

		CString list = list_text;
		list.MakeUpper();

		const char *pStr = list.GetString();

		while (*pStr)
		{
			if (pStr[0] == '/' && pStr[1] == '/')
			{
				int index = CString(pStr).FindOneOf("\r\n");
				if (index == -1) break;
				pStr += index;
			}

			token_card0_rank = CardIdentHelper(*pStr++);

			if (token_card0_rank == -1)
				continue;

			token_card1_rank = CardIdentHelper(*pStr++);

			if (token_card0_rank == -1)
				continue;

			// make card0 have the higher rank
			if (token_card0_rank < token_card1_rank)
			{
				temp_rank = token_card0_rank;
				token_card0_rank = token_card1_rank;
				token_card1_rank = temp_rank;
			}

			if (*pStr == 'S') // suited
			{
				inlist[token_card0_rank][token_card1_rank] = true;
				pStr++;
			}
			else  // offsuit or pair
			{
				inlist[token_card1_rank][token_card0_rank] = true;
			}
		}

	LeaveCriticalSection(&cs_formula);

	__SEH_LOGFATAL("CFormula::ParseHandList :\n");
}



void CopyFormula(const SFormula *f, SFormula *t)
{
	__SEH_HEADER

	SHandList		list;
	SFunction		func;
	int				from_count = 0, to_count = 0, from_iter = 0, to_iter = 0;
	bool			addit = false, deleteit = false;

	// Init locals
	list.list = "";
	list.list_text = "";
	func.cache = 0.;
	func.dirty = false;
	func.fresh = false;
	func.func = "";
	func.func_text = "";

	// handle deleted udfs
	to_count = (int) t->mFunction.GetSize();
	for (to_iter=0; to_iter<to_count; to_iter++)
	{
		from_count = (int) f->mFunction.GetSize();
		deleteit = true;
		for (from_iter=0; from_iter<from_count; from_iter++)
		{
			if (t->mFunction[to_iter].func == f->mFunction[from_iter].func)
			{
				deleteit = false;
				from_iter=from_count+1;
			}
		}
		if (deleteit)
		{
			t->mFunction.RemoveAt(to_iter, 1);
			to_count = (int) t->mFunction.GetSize();
			to_iter-=1;
		}
	}

	// handle new/changed udfs
	from_count = (int) f->mFunction.GetSize();
	for (from_iter=0; from_iter<from_count; from_iter++)
	{
		to_count = (int) t->mFunction.GetSize();
		addit = true;
		for (to_iter=0; to_iter<to_count; to_iter++)
		{
			if (f->mFunction[from_iter].func == t->mFunction[to_iter].func)
			{
				// changed?
				addit = false;
				if (f->mFunction[from_iter].func_text == t->mFunction[to_iter].func_text)
				{
					// no change
					t->mFunction[to_iter].dirty = false;
					t->mFunction[to_iter].fresh = false;
				}
				else
				{
					// yup, it changed
					t->mFunction[to_iter].func_text = f->mFunction[from_iter].func_text;
					t->mFunction[to_iter].dirty = true;
					t->mFunction[to_iter].fresh = false;
					t->mFunction[to_iter].cache = 0.0;
				}
				to_iter = to_count+1;
			}
		}

		// new
		if (addit)
		{
			func.func = f->mFunction[from_iter].func;
			func.func_text = f->mFunction[from_iter].func_text;
			func.dirty = true;
			func.fresh = false;
			func.cache = 0.0;
			t->mFunction.Add(func);
		}
	}

	// Copy numbers
	t->dBankroll = f->dBankroll;
	t->dDefcon = f->dDefcon;
	t->dRake = f->dRake;
	t->dNit = f->dNit;

	// Copy hand lists
	t->mHandList.RemoveAll();
	from_count = (int) f->mHandList.GetSize();
	for (from_iter=0; from_iter<from_count; from_iter++)
	{
		list.list = f->mHandList[from_iter].list;
		list.list_text = f->mHandList[from_iter].list_text;
		t->mHandList.Add(list);
	}

	__SEH_LOGFATAL("::CopyFormula :\n");
}