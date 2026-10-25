#pragma once

namespace ParamID
{
constexpr const char* oVol[4]    = { "o0vol", "o1vol", "o2vol", "o3vol" };
constexpr const char* oPan[4]    = { "o0pan", "o1pan", "o2pan", "o3pan" };
constexpr const char* oCoarse[4] = { "o0coarse", "o1coarse", "o2coarse", "o3coarse" };
constexpr const char* oFine[4]   = { "o0fine", "o1fine", "o2fine", "o3fine" };
constexpr const char* oPhase[4]  = { "o0phase", "o1phase", "o2phase", "o3phase" };
constexpr const char* oWidth[4]  = { "o0width", "o1width", "o2width", "o3width" };
constexpr const char* oType[4]   = { "o0type", "o1type", "o2type", "o3type" };

constexpr const char* fm[4][4] = {
    { "fm00", "fm01", "fm02", "fm03" },
    { "fm10", "fm11", "fm12", "fm13" },
    { "fm20", "fm21", "fm22", "fm23" },
    { "fm30", "fm31", "fm32", "fm33" }
};
constexpr const char* fmEnvDepth = "fmEnvDepth";
constexpr const char* fmRecipe   = "fmRecipe";

constexpr const char* formA     = "formA";
constexpr const char* formB     = "formB";
constexpr const char* formMorph = "formMorph";
constexpr const char* formTrack = "formTrack";
constexpr const char* formMix   = "formMix";
constexpr const char* vowel     = "vowel";

constexpr const char* filtType = "filtType";
constexpr const char* cutoff   = "cutoff";
constexpr const char* res      = "res";
constexpr const char* filtDrive= "filtDrive";
constexpr const char* keyTrack = "keyTrack";
constexpr const char* filtEnv  = "filtEnv";

constexpr const char* distType = "distType";
constexpr const char* distDrive= "distDrive";
constexpr const char* distTone = "distTone";
constexpr const char* distMix  = "distMix";

constexpr const char* eAtk[6] = { "e0atk", "e1atk", "e2atk", "e3atk", "e4atk", "e5atk" };
constexpr const char* eDec[6] = { "e0dec", "e1dec", "e2dec", "e3dec", "e4dec", "e5dec" };
constexpr const char* eSus[6] = { "e0sus", "e1sus", "e2sus", "e3sus", "e4sus", "e5sus" };
constexpr const char* eRel[6] = { "e0rel", "e1rel", "e2rel", "e3rel", "e4rel", "e5rel" };

constexpr const char* lShape[4] = { "l0shape", "l1shape", "l2shape", "l3shape" };
constexpr const char* lRate[4]  = { "l0rate", "l1rate", "l2rate", "l3rate" };
constexpr const char* lSync[4]  = { "l0sync", "l1sync", "l2sync", "l3sync" };
constexpr const char* lAmt[4]   = { "l0amt", "l1amt", "l2amt", "l3amt" };

constexpr const char* macro[7] = {
    "size", "wetness", "anger", "alien", "squelch", "rip", "toxicity"
};

constexpr const char* polyMode = "polyMode";
constexpr const char* glide    = "glide";
constexpr const char* bendRange = "bendRange";
constexpr const char* voices   = "voices";
constexpr const char* level    = "level";
constexpr const char* randMode = "randMode";

constexpr const char* mxSrc[32] = {
    "m0s","m1s","m2s","m3s","m4s","m5s","m6s","m7s","m8s","m9s",
    "m10s","m11s","m12s","m13s","m14s","m15s","m16s","m17s","m18s","m19s",
    "m20s","m21s","m22s","m23s","m24s","m25s","m26s","m27s","m28s","m29s",
    "m30s","m31s"
};
constexpr const char* mxDst[32] = {
    "m0d","m1d","m2d","m3d","m4d","m5d","m6d","m7d","m8d","m9d",
    "m10d","m11d","m12d","m13d","m14d","m15d","m16d","m17d","m18d","m19d",
    "m20d","m21d","m22d","m23d","m24d","m25d","m26d","m27d","m28d","m29d",
    "m30d","m31d"
};
constexpr const char* mxAmt[32] = {
    "m0a","m1a","m2a","m3a","m4a","m5a","m6a","m7a","m8a","m9a",
    "m10a","m11a","m12a","m13a","m14a","m15a","m16a","m17a","m18a","m19a",
    "m20a","m21a","m22a","m23a","m24a","m25a","m26a","m27a","m28a","m29a",
    "m30a","m31a"
};
constexpr const char* mxOn[32] = {
    "m0n","m1n","m2n","m3n","m4n","m5n","m6n","m7n","m8n","m9n",
    "m10n","m11n","m12n","m13n","m14n","m15n","m16n","m17n","m18n","m19n",
    "m20n","m21n","m22n","m23n","m24n","m25n","m26n","m27n","m28n","m29n",
    "m30n","m31n"
};

constexpr const char* eqOn = "eqOn";
constexpr const char* eqLo = "eqLo";
constexpr const char* eqMid= "eqMid";
constexpr const char* eqHi = "eqHi";
constexpr const char* cmpOn= "cmpOn";
constexpr const char* cmpTh= "cmpTh";
constexpr const char* cmpRt= "cmpRt";
constexpr const char* dlyOn= "dlyOn";
constexpr const char* dlyT = "dlyT";
constexpr const char* dlyFb= "dlyFb";
constexpr const char* revOn= "revOn";
constexpr const char* revSz= "revSz";
constexpr const char* revDm= "revDm";
constexpr const char* choOn= "choOn";
constexpr const char* choRt= "choRt";
constexpr const char* choDp= "choDp";
constexpr const char* phOn = "phOn";
constexpr const char* phRt = "phRt";
constexpr const char* phDp = "phDp";
constexpr const char* flOn = "flOn";
constexpr const char* flRt = "flRt";
constexpr const char* flFb = "flFb";
constexpr const char* shOn = "shOn";
constexpr const char* shHz = "shHz";
constexpr const char* grOn = "grOn";
constexpr const char* grSz = "grSz";
constexpr const char* grDn = "grDn";
constexpr const char* fxSlot[9] = {
    "fx0", "fx1", "fx2", "fx3", "fx4", "fx5", "fx6", "fx7", "fx8"
};
}
