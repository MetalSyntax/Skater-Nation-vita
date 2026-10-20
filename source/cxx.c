/**
 * @file cxx.c
 * @brief Guest libstdc++ / libsupc++ imports bound to the VitaSDK libstdc++ (old COW std::string ABI).
 * @details iOS GCC 4.2 and VitaSDK libstdc++ share the Itanium C++ ABI and 32-bit layouts; only the
 *          mangling of size_t differs (iOS: unsigned long 'm', Vita: unsigned int 'j').
 *          Symbols are referenced through asm labels so the C compiler never needs their C++ types.
 *          Generated from the guest import list checked against `nm libstdc++.a`.
 *          File streams: Darwin's basic_filebuf is 0x204 bytes (44-byte mutex, 128-byte mbstate_t) vs 0x74
 *          here. The guest inlines ifstream/ofstream ctors/dtors with its layout, so the vtables/VTTs are
 *          copies with the basic_ios virtual-base offset moved and __basic_file calls re-based (see below).
 */
#include <stdlib.h>

#include "imports.h"
#include "log.h"

extern char cxx_f0[] __asm__("_ZN9__gnu_cxx18__exchange_and_addEPVii");
extern char cxx_f1[] __asm__("_ZNKSs4findEPKcj");
extern char cxx_f2[] __asm__("_ZNKSs4findEcj");
extern char cxx_f3[] __asm__("_ZNKSs5c_strEv");
extern char cxx_f4[] __asm__("_ZNKSs6lengthEv");
extern char cxx_f5[] __asm__("_ZNKSs6substrEjj");
extern char cxx_f6[] __asm__("_ZNKSs7compareEPKc");
extern char cxx_f7[] __asm__("_ZNKSs7compareERKSs");
extern char cxx_f9[] __asm__("_ZNKSt15basic_stringbufIcSt11char_traitsIcESaIcEE3strEv");
extern char cxx_f10[] __asm__("_ZNKSt9basic_iosIcSt11char_traitsIcEE5widenEc");
extern char cxx_f11[] __asm__("_ZNSaIcEC1Ev");
extern char cxx_f12[] __asm__("_ZNSaIcED1Ev");
extern char cxx_f13[] __asm__("_ZNSo9_M_insertIlEERSoT_");
extern char cxx_f14[] __asm__("_ZNSolsEi");
extern char cxx_f15[] __asm__("_ZNSs2atEj");
extern char cxx_f16[] __asm__("_ZNSs4_Rep10_M_destroyERKSaIcE");
extern char cxx_d17[] __asm__("_ZNSs4_Rep20_S_empty_rep_storageE");
extern char cxx_f18[] __asm__("_ZNSs6appendEPKcj");
extern char cxx_f19[] __asm__("_ZNSs6appendERKSs");
extern char cxx_f20[] __asm__("_ZNSs6appendEjc");
extern char cxx_f21[] __asm__("_ZNSs6assignEPKcj");
extern char cxx_f22[] __asm__("_ZNSs6assignERKSs");
extern char cxx_f23[] __asm__("_ZNSs6insertEjRKSsjj");
extern char cxx_f24[] __asm__("_ZNSs6resizeEjc");
extern char cxx_f25[] __asm__("_ZNSs7reserveEj");
extern char cxx_f26[] __asm__("_ZNSs9_M_mutateEjjj");
extern char cxx_f27[] __asm__("_ZNSs9push_backEc");
extern char cxx_f28[] __asm__("_ZNSsC1EPKcRKSaIcE");
extern char cxx_f29[] __asm__("_ZNSsC1EPKcjRKSaIcE");
extern char cxx_f30[] __asm__("_ZNSsC1ERKSs");
extern char cxx_f31[] __asm__("_ZNSsC1ERKSsjj");
extern char cxx_f32[] __asm__("_ZNSsC1Ev");
extern char cxx_f33[] __asm__("_ZNSsD1Ev");
extern char cxx_f34[] __asm__("_ZNSsD2Ev");
extern char cxx_f35[] __asm__("_ZNSsaSEPKc");
extern char cxx_f36[] __asm__("_ZNSsaSERKSs");
extern char cxx_f37[] __asm__("_ZNSsixEj");
extern char cxx_f39[] __asm__("_ZNSt13basic_filebufIcSt11char_traitsIcEE4openEPKcSt13_Ios_Openmode");
extern char cxx_f40[] __asm__("_ZNSt13basic_filebufIcSt11char_traitsIcEE5closeEv");
extern char cxx_f41[] __asm__("_ZNSt13basic_filebufIcSt11char_traitsIcEEC1Ev");
extern char cxx_f42[] __asm__("_ZNSt15_List_node_base4hookEPS_");
extern char cxx_f43[] __asm__("_ZNSt15_List_node_base4swapERS_S0_");
extern char cxx_f44[] __asm__("_ZNSt15_List_node_base6unhookEv");
extern char cxx_f45[] __asm__("_ZNSt15_List_node_base8transferEPS_S0_");
extern char cxx_f46[] __asm__("_ZNSt19basic_ostringstreamIcSt11char_traitsIcESaIcEEC1ESt13_Ios_Openmode");
extern char cxx_f47[] __asm__("_ZNSt19basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev");
extern char cxx_f48[] __asm__("_ZNSt6localeD1Ev");
extern char cxx_f49[] __asm__("_ZNSt8ios_base4InitC1Ev");
extern char cxx_f50[] __asm__("_ZNSt8ios_base4InitD1Ev");
extern char cxx_f51[] __asm__("_ZNSt8ios_baseC2Ev");
extern char cxx_f52[] __asm__("_ZNSt8ios_baseD2Ev");
extern char cxx_f53[] __asm__("_ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E");
extern char cxx_f54[] __asm__("_ZNSt9basic_iosIcSt11char_traitsIcEE5clearESt12_Ios_Iostate");
extern char cxx_f55[] __asm__("_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_i");
extern char cxx_f56[] __asm__("_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base");
extern char cxx_f57[] __asm__("_ZSt18_Rb_tree_incrementPKSt18_Rb_tree_node_base");
extern char cxx_f58[] __asm__("_ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base");
extern char cxx_f59[] __asm__("_ZSt28_Rb_tree_rebalance_for_erasePSt18_Rb_tree_node_baseRS_");
extern char cxx_f60[] __asm__("_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_");
extern char cxx_f61[] __asm__("_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_");
extern char cxx_f62[] __asm__("_ZSt7getlineIcSt11char_traitsIcESaIcEERSt13basic_istreamIT_T0_ES7_RSbIS4_S5_T1_ES4_");
extern char cxx_d63[] __asm__("_ZSt7nothrow");
extern char cxx_d64[] __asm__("_ZTTSt14basic_ifstreamIcSt11char_traitsIcEE");
extern char cxx_d65[] __asm__("_ZTTSt14basic_ofstreamIcSt11char_traitsIcEE");
extern char cxx_d66[] __asm__("_ZTVN10__cxxabiv117__class_type_infoE");
extern char cxx_d67[] __asm__("_ZTVN10__cxxabiv120__si_class_type_infoE");
extern char cxx_d68[] __asm__("_ZTVN10__cxxabiv121__vmi_class_type_infoE");
extern char cxx_d69[] __asm__("_ZTVSt13basic_filebufIcSt11char_traitsIcEE");
extern char cxx_d70[] __asm__("_ZTVSt14basic_ifstreamIcSt11char_traitsIcEE");
extern char cxx_d71[] __asm__("_ZTVSt14basic_ofstreamIcSt11char_traitsIcEE");
extern char cxx_d72[] __asm__("_ZTVSt15basic_streambufIcSt11char_traitsIcEE");
extern char cxx_d73[] __asm__("_ZTVSt9basic_iosIcSt11char_traitsIcEE");
extern char cxx_f74[] __asm__("_ZdaPv");
extern char cxx_f75[] __asm__("_ZdlPv");
extern char cxx_f76[] __asm__("_ZdlPvRKSt9nothrow_t");
extern char cxx_f77[] __asm__("_Znaj");
extern char cxx_f78[] __asm__("_Znwj");
extern char cxx_f79[] __asm__("_ZnwjRKSt9nothrow_t");
extern char cxx_f80[] __asm__("__cxa_begin_catch");
extern char cxx_f81[] __asm__("__cxa_end_catch");
extern char cxx_f82[] __asm__("__cxa_rethrow");
extern char cxx_f83[] __asm__("__dynamic_cast");

extern const int32_t cxx_ctv_if[] __asm__("_ZTCSt14basic_ifstreamIcSt11char_traitsIcEE0_Si");
extern const int32_t cxx_ctv_of[] __asm__("_ZTCSt14basic_ofstreamIcSt11char_traitsIcEE0_So");

// Guest filebuf keeps __basic_file at +0x4C, host at +0x24.
#define GUEST_FILE_OFF 0x28
extern int cxx_is_open_host(void *f) __asm__("_ZNKSt12__basic_fileIcE7is_openEv");
extern void cxx_basic_file_dtor_host(void *f) __asm__("_ZNSt12__basic_fileIcED1Ev");
static int cxx_is_open(char *f) { return cxx_is_open_host(f - GUEST_FILE_OFF); }
static void cxx_basic_file_dtor(char *f) { cxx_basic_file_dtor_host(f - GUEST_FILE_OFF); }

// 10-word vtable groups (vbase offset, vcall/offset_to_top = -vbase); VTT has 4 entries.
typedef struct { int32_t vt[10], ctv[10]; uintptr_t vtt[4]; } guest_fstream;
static guest_fstream cxx_ifs, cxx_ofs;

static void fix_group(int32_t *dst, const int32_t *src, int32_t host, int32_t guest) {
    for (int i = 0; i < 10; i++)
        dst[i] = src[i] == host ? guest : src[i] == -host ? -guest : src[i];
}

static void fix_fstream(guest_fstream *g, const int32_t *vt, const int32_t *ctv, const uintptr_t *vtt,
                        int32_t host, int32_t guest) {
    fix_group(g->vt, vt, host, guest);
    fix_group(g->ctv, ctv, host, guest);
    for (int i = 0; i < 4; i++) {
        uintptr_t e = vtt[i];
        if (e - (uintptr_t)vt < sizeof(g->vt))
            g->vtt[i] = (uintptr_t)g->vt + (e - (uintptr_t)vt);
        else if (e - (uintptr_t)ctv < sizeof(g->ctv))
            g->vtt[i] = (uintptr_t)g->ctv + (e - (uintptr_t)ctv);
        else
            fatal_error("cxx: unexpected VTT entry %d", i);
    }
}

// ifstream: vptr + _M_gcount + filebuf; ofstream: vptr + filebuf.
__attribute__((constructor)) static void cxx_fstream_init(void) {
    fix_fstream(&cxx_ifs, (const int32_t *)cxx_d70, cxx_ctv_if, (const uintptr_t *)cxx_d64, 8 + 0x74, 8 + 0x204);
    fix_fstream(&cxx_ofs, (const int32_t *)cxx_d71, cxx_ctv_of, (const uintptr_t *)cxx_d65, 4 + 0x74, 4 + 0x204);
}

// Host C++ exceptions cannot unwind the guest's SjLj frames: report and stop.
static void cxx_throw(const char *what) { fatal_error("libstdc++ threw: %s", what ? what : "bad_alloc"); }

// iOS hands out zero-filled fresh memory and the game relies on it: WorldManager's ctor never sets
// +0x4528, and newlib garbage there crashes SetStatusBarOrientation (run 28).
static void *cxx_new_zeroed(size_t n) {
    void *p = calloc(1, n ? n : 1);
    if (!p)
        cxx_throw(NULL);
    return p;
}
static void *cxx_new_zeroed_nothrow(size_t n, const void *tag) { return calloc(1, n ? n : 1); }

const import_entry cxx_imports[] = {
    F_RAW("__ZN9__gnu_cxx18__exchange_and_addEPVii", cxx_f0),
    F_RAW("__ZNKSs4findEPKcm", cxx_f1),
    F_RAW("__ZNKSs4findEcm", cxx_f2),
    F_RAW("__ZNKSs5c_strEv", cxx_f3),
    F_RAW("__ZNKSs6lengthEv", cxx_f4),
    F_RAW("__ZNKSs6substrEmm", cxx_f5),
    F_RAW("__ZNKSs7compareEPKc", cxx_f6),
    F_RAW("__ZNKSs7compareERKSs", cxx_f7),
    F_RAW("__ZNKSt12__basic_fileIcE7is_openEv", cxx_is_open),
    F_RAW("__ZNKSt15basic_stringbufIcSt11char_traitsIcESaIcEE3strEv", cxx_f9),
    F_RAW("__ZNKSt9basic_iosIcSt11char_traitsIcEE5widenEc", cxx_f10),
    F_RAW("__ZNSaIcEC1Ev", cxx_f11),
    F_RAW("__ZNSaIcED1Ev", cxx_f12),
    F_RAW("__ZNSo9_M_insertIlEERSoT_", cxx_f13),
    F_RAW("__ZNSolsEi", cxx_f14),
    F_RAW("__ZNSs2atEm", cxx_f15),
    F_RAW("__ZNSs4_Rep10_M_destroyERKSaIcE", cxx_f16),
    {"__ZNSs4_Rep20_S_empty_rep_storageE", (uintptr_t)cxx_d17, 1},
    F_RAW("__ZNSs6appendEPKcm", cxx_f18),
    F_RAW("__ZNSs6appendERKSs", cxx_f19),
    F_RAW("__ZNSs6appendEmc", cxx_f20),
    F_RAW("__ZNSs6assignEPKcm", cxx_f21),
    F_RAW("__ZNSs6assignERKSs", cxx_f22),
    F_RAW("__ZNSs6insertEmRKSsmm", cxx_f23),
    F_RAW("__ZNSs6resizeEmc", cxx_f24),
    F_RAW("__ZNSs7reserveEm", cxx_f25),
    F_RAW("__ZNSs9_M_mutateEmmm", cxx_f26),
    F_RAW("__ZNSs9push_backEc", cxx_f27),
    F_RAW("__ZNSsC1EPKcRKSaIcE", cxx_f28),
    F_RAW("__ZNSsC1EPKcmRKSaIcE", cxx_f29),
    F_RAW("__ZNSsC1ERKSs", cxx_f30),
    F_RAW("__ZNSsC1ERKSsmm", cxx_f31),
    F_RAW("__ZNSsC1Ev", cxx_f32),
    F_RAW("__ZNSsD1Ev", cxx_f33),
    F_RAW("__ZNSsD2Ev", cxx_f34),
    F_RAW("__ZNSsaSEPKc", cxx_f35),
    F_RAW("__ZNSsaSERKSs", cxx_f36),
    F_RAW("__ZNSsixEm", cxx_f37),
    F_RAW("__ZNSt12__basic_fileIcED1Ev", cxx_basic_file_dtor),
    F_RAW("__ZNSt13basic_filebufIcSt11char_traitsIcEE4openEPKcSt13_Ios_Openmode", cxx_f39),
    F_RAW("__ZNSt13basic_filebufIcSt11char_traitsIcEE5closeEv", cxx_f40),
    F_RAW("__ZNSt13basic_filebufIcSt11char_traitsIcEEC1Ev", cxx_f41),
    F_RAW("__ZNSt15_List_node_base4hookEPS_", cxx_f42),
    F_RAW("__ZNSt15_List_node_base4swapERS_S0_", cxx_f43),
    F_RAW("__ZNSt15_List_node_base6unhookEv", cxx_f44),
    F_RAW("__ZNSt15_List_node_base8transferEPS_S0_", cxx_f45),
    F_RAW("__ZNSt19basic_ostringstreamIcSt11char_traitsIcESaIcEEC1ESt13_Ios_Openmode", cxx_f46),
    F_RAW("__ZNSt19basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev", cxx_f47),
    F_RAW("__ZNSt6localeD1Ev", cxx_f48),
    F_RAW("__ZNSt8ios_base4InitC1Ev", cxx_f49),
    F_RAW("__ZNSt8ios_base4InitD1Ev", cxx_f50),
    F_RAW("__ZNSt8ios_baseC2Ev", cxx_f51),
    F_RAW("__ZNSt8ios_baseD2Ev", cxx_f52),
    F_RAW("__ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E", cxx_f53),
    F_RAW("__ZNSt9basic_iosIcSt11char_traitsIcEE5clearESt12_Ios_Iostate", cxx_f54),
    F_RAW("__ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_i", cxx_f55),
    F_RAW("__ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base", cxx_f56),
    F_RAW("__ZSt18_Rb_tree_incrementPKSt18_Rb_tree_node_base", cxx_f57),
    F_RAW("__ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base", cxx_f58),
    F_RAW("__ZSt28_Rb_tree_rebalance_for_erasePSt18_Rb_tree_node_baseRS_", cxx_f59),
    F_RAW("__ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_", cxx_f60),
    F_RAW("__ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_", cxx_f61),
    F_RAW("__ZSt7getlineIcSt11char_traitsIcESaIcEERSt13basic_istreamIT_T0_ES7_RSbIS4_S5_T1_ES4_", cxx_f62),
    {"__ZSt7nothrow", (uintptr_t)cxx_d63, 1},
    {"__ZTTSt14basic_ifstreamIcSt11char_traitsIcEE", (uintptr_t)cxx_ifs.vtt, 1},
    {"__ZTTSt14basic_ofstreamIcSt11char_traitsIcEE", (uintptr_t)cxx_ofs.vtt, 1},
    {"__ZTVN10__cxxabiv117__class_type_infoE", (uintptr_t)cxx_d66, 1},
    {"__ZTVN10__cxxabiv120__si_class_type_infoE", (uintptr_t)cxx_d67, 1},
    {"__ZTVN10__cxxabiv121__vmi_class_type_infoE", (uintptr_t)cxx_d68, 1},
    {"__ZTVSt13basic_filebufIcSt11char_traitsIcEE", (uintptr_t)cxx_d69, 1},
    {"__ZTVSt14basic_ifstreamIcSt11char_traitsIcEE", (uintptr_t)cxx_ifs.vt, 1},
    {"__ZTVSt14basic_ofstreamIcSt11char_traitsIcEE", (uintptr_t)cxx_ofs.vt, 1},
    {"__ZTVSt15basic_streambufIcSt11char_traitsIcEE", (uintptr_t)cxx_d72, 1},
    {"__ZTVSt9basic_iosIcSt11char_traitsIcEE", (uintptr_t)cxx_d73, 1},
    F_RAW("__ZdaPv", cxx_f74),
    F_RAW("__ZdlPv", cxx_f75),
    F_RAW("__ZdlPvRKSt9nothrow_t", cxx_f76),
    F_RAW("__Znam", cxx_new_zeroed),
    F_RAW("__Znwm", cxx_new_zeroed),
    F_RAW("__ZnwmRKSt9nothrow_t", cxx_new_zeroed_nothrow),
    F_RAW("___cxa_begin_catch", cxx_f80),
    F_RAW("___cxa_end_catch", cxx_f81),
    F_RAW("___cxa_rethrow", cxx_f82),
    F_RAW("___dynamic_cast", cxx_f83),
    F_RAW("__ZSt17__throw_bad_allocv", cxx_throw),
    F_RAW("__ZSt20__throw_length_errorPKc", cxx_throw),
    F_RAW("__ZSt20__throw_out_of_rangePKc", cxx_throw),
};
const int cxx_imports_count = sizeof(cxx_imports) / sizeof(cxx_imports[0]);
