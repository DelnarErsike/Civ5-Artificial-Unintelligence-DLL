// AuI.h
#pragma once

#ifndef AUI_MODS_H
#define AUI_MODS_H

// Non-AI stuff (still used by AI routines, but could be used elsewhere as well)
// AUI's new GUID
#define AUI_GUID
// Can cache doubles from XML (DatabaseUtility actually supports double-type, don't know why Firaxis didn't bother putting this in for good measure)
#define AUI_CACHE_DOUBLE
// Enables the Binomial Random Number Generator
#define AUI_BINOM_RNG
// Minor Civ tracking (especially useful for cases with no minor civs or lots of minor civs relative to major civs)
#define AUI_MINOR_CIV_RATIO



// GlobalDefines (GD) wrappers
// INT
#define GD_INT_DECL(name)       int m_i##name
#define GD_INT_DEF(name)        inline int get##name() { return m_i##name; }
#define GD_INT_INIT(name, def)  m_i##name(def)
#define GD_INT_CACHE(name)      m_i##name = getDefineINT(#name)
#define GD_INT_GET(name)        GC.get##name()
// FLOAT
#define GD_FLOAT_DECL(name)       float m_f##name
#define GD_FLOAT_DEF(name)        inline float get##name() { return m_f##name; }
#define GD_FLOAT_INIT(name, def)  m_f##name(def)
#define GD_FLOAT_CACHE(name)      m_f##name = getDefineFLOAT(#name)
#define GD_FLOAT_GET(name)        GC.get##name()
// DOUBLE (high precision, but much slower than float)
#ifdef AUI_CACHE_DOUBLE
#define GD_DOUBLE_DECL(name)       double m_d##name
#define GD_DOUBLE_DEF(name)        inline double get##name() { return m_d##name; }
#define GD_DOUBLE_INIT(name, def)  m_d##name(def)
#define GD_DOUBLE_CACHE(name)      m_d##name = getDefineDOUBLE(#name)
#define GD_DOUBLE_GET(name)        GC.get##name()
#endif

#endif