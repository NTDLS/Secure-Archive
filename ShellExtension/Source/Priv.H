#ifndef Priv_H
#define Priv_H
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef STRICT
#define STRICT
#endif

#define INC_OLE2        // WIN32, get ole2 from windows.h

#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
