#ifndef __UBASE_H__
#define __UBASE_H__

#ifdef UBASE_EXPORTS
#define UBASE_API __declspec(dllexport)
#else
#define UBASE_API __declspec(dllimport)
#endif

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName&);               \
	void operator=(const TypeName&)

#endif