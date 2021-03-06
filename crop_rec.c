#include <dryos.h>
#include <module.h>
#include <config.h>
#include <menu.h>
#include <beep.h>
#include <property.h>
#include <patch.h>
#include <bmp.h>
#include <lvinfo.h>
#include <powersave.h>
#include <raw.h>
#include <fps.h>
#include <shoot.h>
#include <lens.h>

extern WEAK_FUNC(ret_0) unsigned int is_crop_hack_supported();
extern WEAK_FUNC(ret_0) unsigned int movie_crop_hack_enable();
extern WEAK_FUNC(ret_0) unsigned int movie_crop_hack_disable();

#undef CROP_DEBUG

#ifdef CROP_DEBUG
#define dbg_printf(fmt,...) { printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

static int is_digic4 = 0;
static int is_digic5 = 0;
static int is_5D3 = 0;
static int is_6D = 0;
static int is_700D = 0;
static int is_650D = 0;
static int is_100D = 0;
static int is_EOSM = 0;
static int is_basic = 0;

static CONFIG_INT("crop.preset", crop_preset_index, 1);
static CONFIG_INT("crop.shutter_range", shutter_range, 0);
static CONFIG_INT("crop.bitdepth", bitdepth, 0);
static CONFIG_INT("crop.ratios", ratios, 3);
static CONFIG_INT("crop.x3crop", x3crop, 0);
static CONFIG_INT("crop.zoomaid", zoomaid, 1);
static CONFIG_INT("crop.x3toggle", x3toggle, 2);
static CONFIG_INT("crop.set_25fps", set_25fps, 0);
static CONFIG_INT("crop.HDR_iso_a", HDR_iso_a, 0);
static CONFIG_INT("crop.HDR_iso_b", HDR_iso_b, 0);
static CONFIG_INT("crop.isoauto", isoauto, 0);
static CONFIG_INT("crop.isoclimb", isoclimb, 0);
static CONFIG_INT("crop.timelapse", timelapse, 0);
static CONFIG_INT("crop.slowshutter", slowshutter, 0);
static CONFIG_INT("crop.presets", presets, 0);

enum crop_preset {
    CROP_PRESET_OFF = 0,
    CROP_PRESET_OFF_eosm = 0,
    CROP_PRESET_3X,
    CROP_PRESET_3X_TALL,
    CROP_PRESET_3K,
    CROP_PRESET_4K_HFPS,
    CROP_PRESET_UHD,
    CROP_PRESET_FULLRES_LV,
    CROP_PRESET_3x3_1X,
    CROP_PRESET_3x3_1X_45p,
    CROP_PRESET_3x3_1X_48p,
    CROP_PRESET_3x3_1X_50p,
    CROP_PRESET_3x3_1X_60p,
    CROP_PRESET_3x1,
    CROP_PRESET_40_FPS,
    CROP_PRESET_CENTER_Z,
    CROP_PRESET_mv1080_mv720p,
    CROP_PRESET_1x3,
    CROP_PRESET_1x3_17fps,
    CROP_PRESET_anamorphic_rewired_100D,
    CROP_PRESET_1080K_100D,
    CROP_PRESET_3xcropmode_100D,
    CROP_PRESET_3x3_1X_100D,
    CROP_PRESET_mv1080p_mv720p_100D,
    CROP_PRESET_2K_100D,
    CROP_PRESET_3K_100D,
    CROP_PRESET_4K_100D,
    CROP_PRESET_4K_3x1_100D,
    CROP_PRESET_5K_3x1_100D,
    CROP_PRESET_3x3_mv1080_EOSM,
    CROP_PRESET_mcm_mv1080_EOSM,
    CROP_PRESET_3x3_mv1080_48fps_EOSM,
    CROP_PRESET_3x1_mv720_50fps_EOSM,
    CROP_PRESET_CENTER_Z_EOSM,
    CROP_PRESET_3x3_1X_EOSM,
    CROP_PRESET_2K_EOSM,
    CROP_PRESET_3K_EOSM,
    CROP_PRESET_4K_EOSM,
    CROP_PRESET_4K_3x1_EOSM,
    CROP_PRESET_5K_3x1_EOSM,
    CROP_PRESET_4K_5x1_EOSM,
    CROP_PRESET_anamorphic_rewired_EOSM,
    CROP_PRESET_anamorphic_EOSM,
    CROP_PRESET_x10_EOSM,
    CROP_PRESET_2520_1418_700D,
    CROP_PRESET_3K_700D,
    CROP_PRESET_3540_700D,
    CROP_PRESET_UHD_700D,
    CROP_PRESET_4K_700D,
    CROP_PRESET_FULLRES_LV_700D,
    CROP_PRESET_CENTER_Z_700D,
    CROP_PRESET_3x3_mv1080_700D,
    CROP_PRESET_3x3_mv1080_48fps_700D,
    CROP_PRESET_anamorphic_700D,
    CROP_PRESET_2520_1418_650D,
    CROP_PRESET_3K_650D,
    CROP_PRESET_28K_650D,
    CROP_PRESET_3540_650D,
    CROP_PRESET_UHD_650D,
    CROP_PRESET_4K_650D,
    CROP_PRESET_FULLRES_LV_650D,
    CROP_PRESET_CENTER_Z_650D,
    CROP_PRESET_3x3_mv1080_650D,
    CROP_PRESET_3x3_mv1080_48fps_650D,
    CROP_PRESET_anamorphic_650D,
    CROP_PRESET_HD_6D,
    CROP_PRESET_HD100_6D,
    CROP_PRESET_HD75_6D,
    CROP_PRESET_HD50_6D,
    CROP_PRESET_2K_6D,
    CROP_PRESET_ULTRA_HD_6D,
    CROP_PRESET_4K_6D,
    CROP_PRESET_FullRes_6D,
    CROP_PRESET_1x3_6D,
    CROP_PRESET_1x3_24FPS_6D,
    CROP_PRESET_5K_6D,
    CROP_PRESET_HD2_6D,
    CROP_PRESET_3x3_1X_6D,
    CROP_PRESET_3x3_1X_48p_6D,
    NUM_CROP_PRESETS
};

/* presets are not enabled right away (we need to go to play mode and back)
 * so we keep two variables: what's selected in menu and what's actually used.
 * note: the menu choices are camera-dependent */
static enum crop_preset crop_preset = 0;

/* must be assigned in crop_rec_init */
static enum crop_preset * crop_presets = 0;

/* current menu selection (*/
#define CROP_PRESET_MENU crop_presets[crop_preset_index]

/* menu choices for 5D3 */
static enum crop_preset crop_presets_5d3[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_1x3,
    CROP_PRESET_3K,
    CROP_PRESET_3X,
    CROP_PRESET_CENTER_Z,
    CROP_PRESET_3x3_1X_45p,
    CROP_PRESET_3x3_1X_48p,
    CROP_PRESET_3x3_1X_50p,
    CROP_PRESET_3x3_1X_60p,
    CROP_PRESET_UHD,
    CROP_PRESET_4K_HFPS,
    CROP_PRESET_FULLRES_LV,
    CROP_PRESET_mv1080_mv720p,
    CROP_PRESET_1x3_17fps,
    //CROP_PRESET_3X_TALL,
    //CROP_PRESET_1x3,
    //CROP_PRESET_3x1,
    //CROP_PRESET_40_FPS,
};

static const char * crop_choices_5d3[] = {
    "OFF",
    "anamorphic",
    "3K 1:1",
    "1920 1:1",
    "3.5K 1:1 centered x5",
    "1080p 45fps",
    "1050p 48fps",
    "1920 50fps",
    "1920 60fps",
    "UHD 1:1",
    "4K 1:1 half-fps",
    "Full-res LiveView",
    "mv1080p_mv720p",
    "1x3_17fps_1920x3240",
    //"1920 1:1 tall",
    //"1x3 binning",
    //"3x1 binning",      /* doesn't work well */
    //"40 fps",
};

static const char crop_choices_help_5d3[] =
"Change 1080p and 720p movie modes into crop modes (select one)";

static const char crop_choices_help2_5d3[] =
"\n"
"1x3 binning anamorphic\n"
"1:1 3K crop (3072x1920 @ 24p, square raw pixels, preview broken)\n"
"1:1 sensor readout (square raw pixels, 3x crop, good preview in 1080p)\n"
"1:1 readout in x5 zoom mode (centered raw, high res, cropped preview)\n"
"1920x1080 @ 45p, 3x3 binning (50/60 FPS in Canon menu)\n"
"1920x1050 @ 48p, 3x3 binning (50/60 FPS in Canon menu)\n"
"1920x960 @ 50p, 3x3 binning (50/60 FPS in Canon menu)\n"
"1920x804 @ 60p, 3x3 binning (50/60 FPS in Canon menu)\n"
"1:1 4K UHD crop (3840x1600 @ 24p, square raw pixels, preview broken)\n"
"1:1 4K crop (4096x3072 @ 12.5 fps, half frame rate, preview broken)\n"
"Full resolution LiveView (5796x3870 @ 7.4 fps, 5784x3864, preview broken)\n"
"mv1080_mv720p clean"
"1x3_17fps binning: read all lines, bin every 3 columns (extreme anamorphic)\n"
"1x3 binning: read all lines, bin every 3 columns (extreme anamorphic)\n"
"1:1 crop, higher vertical resolution (1920x1920 @ 24p, cropped preview)\n"
//"3x1 binning: bin every 3 lines, read all columns (extreme anamorphic)\n"
"FPS override test\n";

/* menu choices for 100D */
static enum crop_preset crop_presets_100d[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_anamorphic_rewired_100D,
    CROP_PRESET_1080K_100D,
    CROP_PRESET_3xcropmode_100D,
    CROP_PRESET_3x3_1X_100D,
    CROP_PRESET_mv1080p_mv720p_100D,
    CROP_PRESET_2K_100D,
    CROP_PRESET_3K_100D,
    CROP_PRESET_4K_100D,
    // CROP_PRESET_4K_3x1_100D,
    // CROP_PRESET_5K_3x1_100D,
};

static const char * crop_choices_100d[] = {
    "OFF",
    "anamorphic rewired",
    "2K 2520x1080p",
    "3x crop mode",
    "3x3 720p",
    "mv1080p_mv720p mode",
    "2.5K 2520x1418",
    "3K 3000x1432",
    "4K 4080x3000",
    //  "4K 3x1 24fps",
    //  "5K 3x1 24fps",
};

static const char crop_choices_help_100d[] =
"Refresh regs if needed by open and exit ML menu...";
static const char crop_choices_help2_100d[] =
"\n"
"1x3 binning modes(anamorphic) rewired\n"
"1:1 2.5K x5crop, real time preview\n"
"1:1 Movie crop mode\n"
"3x3 binning in 720p\n"
"regular mv1080p mode\n"
"2K x5 crop, framing preview\n"
"1:1 3K x5crop, framing preview\n"
"1:1 4K x5 crop, framing preview\n";
//  "3:1 4K x5 crop, framing preview\n"
//  "3:1 5K x5 crop, framing preview\n"

/* menu choices for EOSM */
static enum crop_preset crop_presets_eosm[] = {
    CROP_PRESET_OFF_eosm,
    CROP_PRESET_mcm_mv1080_EOSM,
    CROP_PRESET_3x3_mv1080_48fps_EOSM,
    CROP_PRESET_3x3_mv1080_EOSM,
    CROP_PRESET_3x1_mv720_50fps_EOSM,
    CROP_PRESET_CENTER_Z_EOSM,
    CROP_PRESET_2K_EOSM,
    CROP_PRESET_3K_EOSM,
    CROP_PRESET_4K_EOSM,
    CROP_PRESET_anamorphic_rewired_EOSM,
    CROP_PRESET_anamorphic_EOSM,
    // CROP_PRESET_4K_3x1_EOSM,
    // CROP_PRESET_5K_3x1_EOSM,
    // CROP_PRESET_4K_5x1_EOSM,
    // CROP_PRESET_3x3_1X_EOSM,
};

static const char * crop_choices_eosm[] = {
    "OFF",
    "mv1080p MCM rewire",
    "mv1080p 1736x976 46/48fps",
    "mv1080p 1736x1158",
    "mv720p 1736x694 50fps",
    "2.5K 1:1 centered",
    "2.5K 2520x1418",
    "3K 3032x1436",
    "4K 4080x3000",
    "4K anamorphic rewired",
    "5K anamorphic",
    // "4K 3x1 24fps",
    // "5K 3x1 24fps",
    // "4K 5x1 24fps",
    // "3x3 720p",
};

static const char crop_choices_help_eosm[] =
"Refresh regs if needed by open and exit ML menu...";

static const char crop_choices_help2_eosm[] =
"\n"
"mv1080p realtime full preview. Works with Canon fps 24/25/30fps\n"
"mv1080p 46/48 fps\n"
"mv1080p bypass mv720p idle mode\n"
"mv720p 50fps 16:9\n"
"1:1 x5 zoom mode(centered raw, cropped preview)\n"
"1:1 2K x5crop, real time preview\n"
"1:1 3K x5crop, framing preview\n"
"1:1 4K x5crop, framing preview\n"
"1x3 binning modes(anamorphic)\n"
"1x3 binning modes(anamorphic)\n";
// "3:1 4K x5crop, framing preview\n"
// "3:1 5K x5crop, framing preview\n"
// "5:1 4K crop squeeze, preview broken\n"
// "3x3 binning in 720p (square pixels in RAW, vertical crop)\n"


/* menu choices for 700D */
static enum crop_preset crop_presets_700d[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_2520_1418_700D,
    CROP_PRESET_3K_700D,
    // CROP_PRESET_3540_700D,
    // CROP_PRESET_UHD_700D,
    CROP_PRESET_4K_700D,
    CROP_PRESET_FULLRES_LV_700D,
    CROP_PRESET_CENTER_Z_700D,
    // CROP_PRESET_3x3_mv1080_700D,
    CROP_PRESET_3x3_mv1080_48fps_700D,
    CROP_PRESET_anamorphic_700D,
};

static const char * crop_choices_700d[] = {
    "OFF",
    "2.5K",
    "3K",
    // "3.5K 1:1",
    // "UHD 1:1 half-fps",
    "4K",
    "Full-res LiveView",
    "2.5K 1:1 centered x5",
    // "mv1080",
    "mv1080p 1736x976 46/48fps",
    "5K anamorphic",
};

static const char crop_choices_help_700d[] =
"Change 1080p and 720p movie modes into crop modes (select one)";

static const char crop_choices_help2_700d[] =
"\n"
"1:1 2K x5crop, real time preview\n"
"1:1 3K x5crop, framing preview\n"
// "1:1 3.5K crop (3540x1080 @ 22p, square raw pixels, preview broken)\n"
// "1:1 4K UHD (3840x2160 @ 12 fps, half frame rate, preview broken)\n"
"1:1 4K x5crop, framing preview\n"
"Full resolution LiveView (preview broken)\n"
"1:1 readout in x5 zoom mode (centered raw, high res, cropped preview)\n"
// "mv1080(liveview stretched) \n"
"mv1080p 46/48 fps\n"
"1x3 binning modes(anamorphic)\n";


/* menu choices for 650D */
static enum crop_preset crop_presets_650d[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_2520_1418_650D,
    CROP_PRESET_3K_650D,
    // CROP_PRESET_3540_650D,
    // CROP_PRESET_UHD_650D,
    CROP_PRESET_4K_650D,
    CROP_PRESET_FULLRES_LV_650D,
    CROP_PRESET_CENTER_Z_650D,
    // CROP_PRESET_3x3_mv1080_650D,
    CROP_PRESET_3x3_mv1080_48fps_650D,
    CROP_PRESET_28K_650D
    CROP_PRESET_anamorphic_650D,
};

static const char * crop_choices_650d[] = {
    "OFF",
    "2.5K",
    "3K",
    // "3.5K 1:1",
    // "UHD 1:1 half-fps",
    "4K",
    "Full-res LiveView",
    "2.5K 1:1 centered x5",
    // "mv1080",
    "mv1080p 1736x976 46/48fps",
    "5K anamorphic",
};

static const char crop_choices_help_650d[] =
"Change 1080p and 720p movie modes into crop modes (select one)";

static const char crop_choices_help2_650d[] =
"\n"
"1:1 2K x5crop, real time preview\n"
"1:1 3K x5crop, framing preview\n"
// "1:1 3.5K crop (3540x1080 @ 22p, square raw pixels, preview broken)\n"
// "1:1 4K UHD (3840x2160 @ 12 fps, half frame rate, preview broken)\n"
"1:1 4K x5crop, framing preview\n"
"Full resolution LiveView (preview broken)\n"
"1:1 readout in x5 zoom mode (centered raw, high res, cropped preview)\n"
// "mv1080(liveview stretched) \n"
"mv1080p 46/48 fps\n"
"1x3 binning modes(anamorphic)\n";

/* menu choices for 6D */
static enum crop_preset crop_presets_6d[] = {
    CROP_PRESET_HD2_6D,
    CROP_PRESET_2K_6D,
    CROP_PRESET_ULTRA_HD_6D,
    //    CROP_PRESET_HD100_6D, //disabled this one, little unstable hit or miss if camera locks and error 70 pops up
    //    CROP_PRESET_HD75_6D,
    //    CROP_PRESET_4K_6D,
    //    CROP_PRESET_5K_6D,
    //    CROP_PRESET_FullRes_6D,
    CROP_PRESET_1x3_6D,
    CROP_PRESET_1x3_24FPS_6D,
    CROP_PRESET_HD50_6D,
    CROP_PRESET_OFF,   // build starts automatically in preset 6, the first time, so  )
    
};

static const char * crop_choices_6d[] = {
    "2480x1396 25fps for 5x zoom mode",
    "2688x1296 25fps for 5x zoom mode",
    "2880x1200 25fps for 5x zoom mode",
    //    "1688x392 100fps for 720p mode",
    //    "1688x560 75fps for 720p mode", //disabled this one, interface reacts really slow, more proof of concept, far from pefect
    //    "Max ISO and Gain mode",
    //    "5120x610 25fps for nonzoom mode",
    //    "5464x3072 2.5fps for 5x zoom mode",
    "1216x2052 1x3 25fps for 5x zoom mode",
    "1640x2144 1x3 24fps for 5x zoom mode",
    "1824x818 50fps for 720p mode",
    "OFF"
    
};

static const char crop_choices_help_6d[] =
"Change 1080p and 720p movie modes into crop modes (select one)";

static const char crop_choices_help2_6d[] =
"USE IN 5x ZOOM MODE WITH LIVEVIEW\n"
"USE IN 5x ZOOM MODE WITH LIVEVIEW\n"
"USE IN 5x ZOOM MODE WITH LIVEVIEW\n"
//    "DONE IN 3x3 MODE NO VERTICAL UNSTRETCH NEEDED\n"
//    "DONE IN 3x3 MODE NO VERTICAL UNSTRETCH NEEDED\n"
//    "NightVision\n"
//    "USE IN 5x ZOOM MODE ONLY ML PREVIEW\n" 
//    "USE IN 5x ZOOM MODE ONLY ML PREVIEW\n"
"USE IN 5x ZOOM MODE ONLY ML PREVIEW\n"
"USE IN 5x ZOOM MODE ONLY ML PREVIEW\n"
"DONE IN 3x3 MODE NO VERTICAL UNSTRETCH NEEDED\n"
"off\n";

/* menu choices for cameras that only have the basic 3x3 crop_rec option */
static enum crop_preset crop_presets_basic[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_3x3_1X,
};

static const char * crop_choices_basic[] = {
    "OFF",
    "3x3 720p",
};

static const char crop_choices_help_basic[] =
"Change 1080p and 720p movie modes into crop modes (one choice)";

static const char crop_choices_help2_basic[] =
"3x3 binning in 720p (square pixels in RAW, vertical crop)";


/* camera-specific parameters */
static uint32_t CMOS_WRITE      = 0;
static uint32_t MEM_CMOS_WRITE  = 0;
static uint32_t ADTG_WRITE      = 0;
static uint32_t MEM_ADTG_WRITE  = 0;
static uint32_t ENGIO_WRITE     = 0;
static uint32_t MEM_ENGIO_WRITE = 0;

/* from SENSOR_TIMING_TABLE (fps-engio.c) or FPS override submenu */
static int fps_main_clock = 0;
static int default_timerA[11]; /* 1080p  1080p  1080p   720p   720p   zoom   crop   crop   crop   crop   crop */
static int default_timerB[11]; /*   24p    25p    30p    50p    60p     x5    24p    25p    30p    50p    60p */
static int default_fps_1k[11] = { 23976, 25000, 29970, 50000, 59940, 29970, 23976, 25000, 29970, 50000, 59940 };

/* video modes */
/* note: zoom mode is identified by checking registers directly */

static int is_1080p()
{
    /* note: on 5D2 and 5D3 (maybe also 6D, not sure),
     * sensor configuration in photo mode is identical to 1080p.
     * other cameras may be different */
    return !is_movie_mode() || video_mode_resolution == 0;
}

static int is_720p()
{
    if (is_EOSM)
    {
        if (lv_dispsize == 1 && !RECORDING_H264)
        {
            return 1;
        }
    }
    
    return is_movie_mode() && video_mode_resolution == 1;
}

static int is_supported_mode()
{
    if (!lv) return 0;
    
    /* no more crashes when selecing photo mode */
    if (!is_movie_mode() || CROP_PRESET_MENU == CROP_PRESET_OFF) return 0;
    
    /* workaround getting below cams working with focus aid */
    static int last_hs_aid = 0;
    if (!get_halfshutter_pressed()) last_hs_aid = get_ms_clock();
    if (get_ms_clock() - last_hs_aid > 300 && get_halfshutter_pressed() && (is_6D || is_5D3) && !RECORDING && (zoomaid == 0x1 || zoomaid == 0x2)) return 0;
    
    switch (crop_preset)
    {
            /* note: zoom check is also covered by check_cmos_vidmode */
            /* (we need to apply CMOS settings before PROP_LV_DISPSIZE fires) */
        case CROP_PRESET_CENTER_Z:
        case CROP_PRESET_CENTER_Z_700D:
        case CROP_PRESET_CENTER_Z_650D:
        case CROP_PRESET_CENTER_Z_EOSM:
            return 1;
            
        case CROP_PRESET_3x3_1X_100D:
            return lv_dispsize != 1 ? is_1080p(): is_720p();
            break;
            
        default:
            return is_1080p() || is_720p();
    }
}

static int32_t  target_yres = 0;
// static int32_t  delta_adtg0 = 0;
// static int32_t  delta_adtg1 = 0;
static int32_t  delta_head3 = 0;
static int32_t  delta_head4 = 0;
static int32_t  reg_713c = 0;
static int32_t  reg_7150 = 0;
static int32_t  reg_6014 = 0;
static int32_t  reg_6008 = 0;
static int32_t  reg_800c = 0;
static int32_t  reg_8000 = 0;
static int32_t  reg_8183 = 0;
static int32_t  reg_8184 = 0;
static int32_t  reg_timing1 = 0;
static int32_t  reg_timing2 = 0;
static int32_t  reg_timing3 = 0;
static int32_t  reg_timing4 = 0;
static int32_t  reg_timing5 = 0;
static int32_t  reg_timing6 = 0;
static int32_t  reg_6824 = 0;
static int32_t  reg_6800_height = 0;
static int32_t  reg_6800_width = 0;
static int32_t  reg_6804_height = 0;
static int32_t  reg_6804_width = 0;
static int32_t  reg_83d4 = 0;
static int32_t  reg_83dc = 0;
static uint32_t cmos1_lo = 0, cmos1_hi = 0;
static uint32_t cmos0 = 0;
static uint32_t cmos1 = 0;
static uint32_t cmos2 = 0;
static uint32_t cmos3 = 0;
static uint32_t cmos4 = 0;
static uint32_t cmos5 = 0;
static uint32_t cmos6 = 0;
static uint32_t cmos7 = 0;
static uint32_t cmos8 = 0;
static uint32_t cmos9 = 0;
static int32_t  reg_skip_left = 0;
static int32_t  reg_skip_right = 0;
static int32_t  reg_skip_top = 0;
static int32_t  reg_skip_bottom = 0;
static int32_t  reg_bl = 0;
static int32_t  reg_gain = 0;
static int crop_patch = 0;
static int crop_patch2 = 0;
static int isopatch = 0;
static int isopatchoff = 1;
//static int preset1 = 1;
//static int preset2 = 1;
//static int preset3 = 1;
static int32_t  reg_8 = 0;
static int32_t  reg_9 = 0;
static int32_t  reg_a = 0;
static int32_t  reg_b = 0;
static int32_t  reg_fe = 0;

/* helper to allow indexing various properties of Canon's video modes */
static inline int get_video_mode_index()
{
    if (lv_dispsize > 1)
    {
        return 5;
    }
    
    if (!is_movie_mode())
    {
        /* FIXME: some cameras may use 50p or 60p */
        return 2;
    }
    
    // if (is_EOSM) Hard to say why this is needed. Fps seems correct without it
    // {
    //     if (lv_dispsize == 1 && !RECORDING_H264)
    //     {
    //         /* EOS M stays in 720p30 during standby (same timer values as with 1080p30) */
    //         return 2;
    //     }
    // }
    
    if (video_mode_crop)
    {
        /* some cameras may have various crop modes, hopefully at most one per FPS */
        return
        (video_mode_fps == 24) ?  6 :
        (video_mode_fps == 25) ?  7 :
        (video_mode_fps == 30) ?  8 :
        (video_mode_fps == 50) ?  9 :
        /* (video_mode_fps == 60) */ 10 ;
    }
    
    /* regular video modes */
    return
    (video_mode_fps == 24) ?  0 :
    (video_mode_fps == 25) ?  1 :
    (video_mode_fps == 30) ?  2 :
    (video_mode_fps == 50) ?  3 :
    /* (video_mode_fps == 60) */ 4 ;
}

/* optical black area sizes */
/* not sure how to adjust them from registers, so... hardcode them here */
static inline void FAST calc_skip_offsets(int * p_skip_left, int * p_skip_right, int * p_skip_top, int * p_skip_bottom)
{
    /* start from LiveView values */
    int skip_left       = 146;
    int skip_right      = 2;
    int skip_top        = 28;
    int skip_bottom     = 0;
    
    if (is_EOSM || is_700D || is_650D)
    {
        skip_left       = 72;
        skip_right      = 0;
        skip_top        = 30;
        skip_bottom     = 0;
    }
    
    if (is_100D)
    {
        skip_left       = 72;
        skip_right      = 0;
        skip_top        = 30;
        skip_bottom     = 0;
    }
    
    if (is_6D)
    {
        skip_left       = 80;
        skip_right      = 8;
        skip_top        = 28;
        skip_bottom     = 0;
    }
    
    switch (crop_preset)
    {
        case CROP_PRESET_FULLRES_LV:
            /* photo mode values */
            skip_left       = 138;
            skip_right      = 2;
            skip_top        = 60;   /* fixme: this is different, why? */
            break;
            
        case CROP_PRESET_3K:
        case CROP_PRESET_UHD:
        case CROP_PRESET_4K_HFPS:
            skip_right      = 0;    /* required for 3840 - tight fit */
            /* fall-through */
            
        case CROP_PRESET_3X_TALL:
            skip_top        = 30;
            break;
            
        case CROP_PRESET_1x3:
            if (ratios == 0x1)
            {
                skip_left       = 206;
                skip_right      = 62;
                skip_top        = 60;
                skip_bottom     = 30;
            }
            if (ratios == 0x1 && set_25fps == 0x1)
            {
                skip_left       = 248;
                skip_right      = 110;
                skip_top        = 60;
                skip_bottom     = 42;
            }
            if (ratios == 0x2)
            {
                skip_left       = 206;
                skip_right      = 62;
                skip_top        = 60;
            }
            if (ratios == 0x2 && set_25fps == 0x1)
            {
                skip_left       = 248;
                skip_right      = 110;
                skip_top        = 60;
            }
            if (ratios == 0x3)
            {
                skip_left       = 426;
                skip_right      = 282;
                skip_top        = 60;
            }
            if (ratios == 0x3 && set_25fps == 0x1)
            {
                skip_left       = 512;
                skip_right      = 270;
                skip_top        = 60;
            }
            break;
            
        case CROP_PRESET_3X:
        case CROP_PRESET_1x3_17fps:
            skip_top        = 60;
            break;
            
        case CROP_PRESET_3x3_1X:
        case CROP_PRESET_3x3_1X_50p:
        case CROP_PRESET_3x3_1X_60p:
        case CROP_PRESET_3x3_1X_100D:
        case CROP_PRESET_3x3_1X_48p:
        case CROP_PRESET_3x3_1X_45p:
            if (is_720p()) skip_top = 0;
            break;
            
        case CROP_PRESET_CENTER_Z_EOSM:
            /* set ratio preset */
            skip_left       = 72;
            skip_right      = 0;
            skip_top        = 28;
            skip_bottom     = 0;
            if (ratios == 0x1)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 26;
            }
            if (ratios == 0x2)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 8;
            }
            if (ratios == 0x3)
            {
                skip_left       = 332;
                skip_right      = 330;
                skip_top        = 28;
                skip_bottom     = 0;
            }
            break;
            
        case CROP_PRESET_2K_EOSM:
            /* set ratio preset */
            if (ratios == 0x1)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 18;
            }
            if (ratios == 0x2)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 0;
            }
            if (ratios == 0x3)
            {
                skip_left       = 234;
                skip_right      = 160;
                skip_top        = 28;
                skip_bottom     = 184;
            }
            break;
            
        case CROP_PRESET_3K_EOSM:
            skip_left       = 72;
            skip_right      = 0;
            skip_top        = 28;
            skip_bottom     = 0;
            if (ratios == 0x1)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 20;
            }
            break;
            
        case CROP_PRESET_3x3_mv1080_EOSM:
            /* set ratio preset */
            if (ratios == 0x1)
            {
                skip_bottom = 432;
                skip_left = 72;
            }
            if (ratios == 0x2)
            {
                skip_bottom = 420;
                skip_left = 72;
            }
            if (ratios == 0x3)
            {
                skip_bottom = 182;
            }
            break;
            
        case CROP_PRESET_mcm_mv1080_EOSM:
            if (ratios == 0x0 && x3crop == 0x0)
            {
                skip_right = 60;
                skip_bottom = 2;
            }
            if (ratios == 0x0 && x3crop == 0x1)
            {
                skip_right = 60;
                skip_bottom = 2;
            }
            if (ratios == 0x1 && x3crop == 0x0)
            {
                skip_right = 60;
                skip_top = 201;
                skip_bottom = 133;
            }
            if (ratios == 0x1 && x3crop == 0x1)
            {
                skip_right = 0;
                skip_top = 172;
                skip_bottom = 184;
            }
            if (ratios == 0x2 && x3crop == 0x0)
            {
                skip_right = 60;
                skip_top = 201;
                skip_bottom = 121;
            }
            if (ratios == 0x2 && x3crop == 0x1)
            {
                skip_right = 0;
                skip_top = 172;
                skip_bottom = 172;
            }
            if (ratios == 0x3 && x3crop == 0x0)
            {
                skip_top = 82;
                skip_right = 60;
                skip_bottom = 2;
            }
            if (ratios == 0x3 && x3crop == 0x1)
            {
                skip_top = 84;
                skip_right = 0;
                skip_bottom = 14;
            }
            break;
            
        case CROP_PRESET_28K_650D:
            skip_left       = 72;
            skip_right      = 0;
            skip_top        = 28;
            skip_bottom     = 0;
            if (ratios == 0x1)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 20;
            }
            if (ratios == 0x3)
            {
                skip_left       = 72;
                skip_right      = 0;
                skip_top        = 28;
                skip_bottom     = 0;
            }
            break;
            
        case CROP_PRESET_3x1_mv720_50fps_EOSM:
            skip_bottom = 2;
            if (ratios == 0x1)
            {
                skip_top = 102;
                skip_bottom = 190;
            }
            if (ratios == 0x2)
            {
                skip_top = 102;
                skip_bottom = 182;
            }
            break;
            
        case CROP_PRESET_3x3_mv1080_48fps_EOSM:
        case CROP_PRESET_3x3_mv1080_48fps_700D:
        case CROP_PRESET_3x3_mv1080_48fps_650D:
            /* see autodetect_black_level exception in raw.c */
            if (ratios == 0x1)
            {
                skip_right = 56;
                skip_left = 98;
                skip_bottom = 40;
            }
            if (ratios == 0x1 && set_25fps == 0x1)
            {
                skip_right = 78;
                skip_left = 120;
                skip_bottom = 56;
            }
            if (ratios == 0x2)
            {
                skip_right = 56;
                skip_left = 98;
                skip_bottom = 26;
            }
            if (ratios == 0x2 && set_25fps == 0x1)
            {
                skip_right = 78;
                skip_left = 120;
                skip_bottom = 44;
            }
            if (ratios == 0x3)
            {
                skip_left = 196;
                skip_right = 112;
                skip_bottom = 132;
            }
            break;
            
        case CROP_PRESET_4K_3x1_EOSM:
            if (ratios == 0x1)
            {
                skip_bottom = 182;
            }
            break;
            
        case CROP_PRESET_anamorphic_rewired_EOSM:
            /* see autodetect_black_level exception in raw.c */
            if (ratios == 0x0)
            {
                skip_right      = 58;
                break;
            }
            /* 2.39:1 */
            if (ratios == 0x1)
            {
                skip_bottom = 54;
                skip_right = 186;
                skip_left = 190;
                if (bitdepth == 0x4)
                {
                    skip_bottom = 54;
                    skip_right = 266;
                    skip_left = 270;
                }
                break;
            }
            /* 2.35:1 */
            if (ratios == 0x2)
            {
                skip_bottom = 20;
                skip_right = 186;
                skip_left = 190;
                if (bitdepth == 0x4)
                {
                    skip_bottom = 20;
                    skip_right = 266;
                    skip_left = 270;
                }
                break;
            }
            if (ratios == 0x3)
            {
                skip_bottom = 20;
                skip_right = 370;
                skip_left = 374;
                if (bitdepth == 0x4)
                {
                    skip_bottom = 20;
                    skip_right = 398;
                    skip_left = 402;
                }
                break;
                
            }
            
        case CROP_PRESET_anamorphic_EOSM:
        case CROP_PRESET_anamorphic_700D:
        case CROP_PRESET_anamorphic_650D:
            /* see autodetect_black_level exception in raw.c */
            skip_bottom = 24;
            if (ratios == 0x1)
            {
                skip_bottom = 54;
                skip_right = 156;
                skip_left = 160;
            }
            if (ratios == 0x2)
            {
                skip_bottom = 20;
                skip_right = 156;
                skip_left = 160;
            }
            if (ratios == 0x3)
            {
                skip_bottom = 0;
                skip_right = 324;
                skip_left = 292;
            }
            break;
            
        case CROP_PRESET_anamorphic_rewired_100D:
            skip_right = 60;
            if (ratios == 0x1)
            {
                skip_bottom = 48;
                skip_right = 175;
                skip_left = 235;
            }
            if (ratios == 0x2)
            {
                skip_bottom = 16;
                skip_right = 175;
                skip_left = 235;
            }
            if (ratios == 0x3)
            {
                skip_bottom = 16;
                skip_right = 340;
                skip_left = 400;
            }
            break;
            
        case CROP_PRESET_4K_5x1_EOSM:
            skip_bottom = 2;
            if (ratios == 0x1)
            {
                skip_bottom = 357;
            }
            if (ratios == 0x2)
            {
                skip_bottom = 247;
            }
            break;
            
        case CROP_PRESET_4K_3x1_100D:
            if (ratios == 0x1)
            {
                skip_bottom = 182;
            }
            break;
            
        case CROP_PRESET_FULLRES_LV_700D:
        case CROP_PRESET_FULLRES_LV_650D:
            skip_bottom     = 4;
            break;
            
        case CROP_PRESET_HD100_6D:
        case CROP_PRESET_HD75_6D:
        case CROP_PRESET_HD50_6D:
        case CROP_PRESET_2K_6D:
        case CROP_PRESET_ULTRA_HD_6D:
        case CROP_PRESET_4K_6D:
        case CROP_PRESET_5K_6D:
        case CROP_PRESET_FullRes_6D:
        case CROP_PRESET_1x3_6D:
        case CROP_PRESET_1x3_24FPS_6D:
        case CROP_PRESET_HD2_6D:
        case CROP_PRESET_3x3_1X_6D:
        case CROP_PRESET_3x3_1X_48p_6D:
            if (is_720p()) skip_top = 28;
            break;
    }
    
    if (p_skip_left)   *p_skip_left    = skip_left + reg_skip_left;
    if (p_skip_right)  *p_skip_right   = skip_right + reg_skip_right;
    if (p_skip_top)    *p_skip_top     = skip_top + reg_skip_top;
    if (p_skip_bottom) *p_skip_bottom  = skip_bottom + reg_skip_bottom;
}

/* to be in sync with 0xC0F06800 */
static int get_top_bar_adjustment()
{
    switch (crop_preset)
    {
        case CROP_PRESET_FULLRES_LV:
            return 0;                   /* 0x10018: photo mode value, unchanged */
        case CROP_PRESET_3x3_1X:
        case CROP_PRESET_3x3_1X_50p:
        case CROP_PRESET_3x3_1X_60p:
        case CROP_PRESET_3x3_1X_100D:
        case CROP_PRESET_3x3_1X_EOSM:
        case CROP_PRESET_3x3_1X_48p:
        case CROP_PRESET_3x3_1X_45p:
            if (is_720p()) return 28;   /* 0x1D0017 from 0x10017 */
            /* fall through */
        default:
            return 30;                  /* 0x1F0017 from 0x10017 */
    }
}

/* Vertical resolution from current unmodified video mode */
/* (active area only, as seen by mlv_lite) */
static inline int get_default_yres()
{
    return
    (video_mode_fps <= 30) ? 1290 : 672;
}

/* skip_top from unmodified video mode (raw.c, LiveView skip offsets) */
static inline int get_default_skip_top()
{
    return
    (video_mode_fps <= 30) ? 28 : 20;
}

/* max resolution for each video mode (trial and error) */
/* it's usually possible to push the numbers a few pixels further,
 * at the risk of corrupted frames */
static int max_resolutions[NUM_CROP_PRESETS][6] = {
    /*   24p   25p   30p   50p   60p   x5 */
    [CROP_PRESET_3X_TALL]       = { 1920, 1728, 1536,  960,  800, 1320 },
    [CROP_PRESET_3x3_1X]        = { 1290, 1290, 1290,  960,  800, 1320 },
    [CROP_PRESET_3x3_1X_50p]    = { 1290, 1290, 1290,  960,  960, 1320 },
    [CROP_PRESET_3x3_1X_60p]    = { 1290, 1290, 1290,  800,  800, 1320 },
    [CROP_PRESET_3x3_1X_48p]    = { 1290, 1290, 1290, 1040, 1040, 1320 },
    [CROP_PRESET_3x3_1X_45p]    = { 1290, 1290, 1290, 1080, 1080, 1320 },
    [CROP_PRESET_3K]            = { 1920, 1728, 1504,  760,  680, 1320 },
    [CROP_PRESET_UHD]           = { 1536, 1472, 1120,  640,  540, 1320 },
    [CROP_PRESET_4K_HFPS]       = { 3072, 3072, 2500, 1440, 1200, 1320 },
    [CROP_PRESET_FULLRES_LV]    = { 3870, 3870, 3870, 3870, 3870, 1320 },
    [CROP_PRESET_mv1080p_mv720p_100D]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_2K_100D]       = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_3K_100D]       = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_4K_3x1_100D]          = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_5K_3x1_100D]          = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_4K_100D]       = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_1080K_100D]    = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_anamorphic_rewired_100D]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3xcropmode_100D]       = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_2K_EOSM]          = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_3K_EOSM]          = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_4K_EOSM]          = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_4K_3x1_EOSM]          = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_5K_3x1_EOSM]          = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_4K_5x1_EOSM]          = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_3x3_mv1080_EOSM]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3x3_mv1080_48fps_EOSM]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3x1_mv720_50fps_EOSM]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_anamorphic_rewired_EOSM]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_anamorphic_EOSM]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3K_700D]         = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_UHD_700D]        = { 1536, 1472, 1120,  640,  540 },
    [CROP_PRESET_4K_700D]         = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_FULLRES_LV_700D] = { 3240, 3240, 3240, 3240, 3240 },
    [CROP_PRESET_3x3_mv1080_48fps_700D]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_anamorphic_700D]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3K_650D]         = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_28K_650D]          = { 1304, 1104,  904,  704,  504 },
    [CROP_PRESET_UHD_650D]        = { 1536, 1472, 1120,  640,  540 },
    [CROP_PRESET_4K_650D]         = { 3072, 3072, 2500, 1440, 1200 },
    [CROP_PRESET_FULLRES_LV_650D] = { 3240, 3240, 3240, 3240, 3240 },
    [CROP_PRESET_3x3_mv1080_48fps_650D]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_anamorphic_650D]  = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3x3_1X_6D]               = { 1216, 1290, 1290,  960,  800 },
    [CROP_PRESET_3x3_1X_48p_6D]           = { 1216, 1290, 1290, 1080, 1040 }, /* 1080p45/48 */
    [CROP_PRESET_HD100_6D]                = { 3306, 1824, 1824,  760,  680 },
    [CROP_PRESET_HD75_6D]                 = { 3306, 1824, 1824,  760,  680 },
    [CROP_PRESET_HD50_6D]                 = { 3306, 1824, 1824,  760,  680 },
    [CROP_PRESET_2K_6D]                   = { 3306, 1728, 1384,  760,  680 },
    [CROP_PRESET_ULTRA_HD_6D]             = { 3306, 1728, 2160,  760,  680 },
    [CROP_PRESET_4K_6D]                   = { 3306, 1728, 2560,  760,  680 },
    [CROP_PRESET_5K_6D]                   = { 3306, 1728, 2560,  760,  680 },
    [CROP_PRESET_FullRes_6D]              = { 3306, 1728, 3490,  760,  680 },
    [CROP_PRESET_1x3_6D]                  = { 3306, 1728, 3490,  760,  680 },
    [CROP_PRESET_1x3_24FPS_6D]            = { 3306, 1728, 3490,  760,  680 },
    [CROP_PRESET_HD2_6D]                  = { 3306, 1728, 3490,  760,  680 },
};

/* 5D3 vertical resolution increments over default configuration */
/* note that first scanline may be moved down by 30 px (see reg_override_top_bar) */
static inline int FAST calc_yres_delta()
{
    int desired_yres = (target_yres) ? target_yres
    : max_resolutions[crop_preset][get_video_mode_index()];
    
    if (desired_yres)
    {
        /* user override */
        int skip_top;
        calc_skip_offsets(0, 0, &skip_top, 0);
        int default_yres = get_default_yres();
        int default_skip_top = get_default_skip_top();
        int top_adj = get_top_bar_adjustment();
        return desired_yres - default_yres + skip_top - default_skip_top + top_adj;
    }
    
    ASSERT(0);
    return 0;
}

#define YRES_DELTA calc_yres_delta()


static int cmos_vidmode_ok = 0;

/* return value:
 *  1: registers checked and appear OK (1080p/720p video mode)
 *  0: registers checked and they are not OK (other video mode)
 * -1: registers not checked
 */
static int FAST check_cmos_vidmode(uint16_t* data_buf)
{
    int ok = 1;
    int found = 1;
    while (*data_buf != 0xFFFF)
    {
        int reg = (*data_buf) >> 12;
        int value = (*data_buf) & 0xFFF;
        
        if (is_5D3)
        {
            if (reg == 1)
            {
                found = 1;
                
                switch (crop_preset)
                {
                    case CROP_PRESET_CENTER_Z:
                    {
                        /* detecting the zoom mode is tricky;
                         * let's just exclude 1080p and 720p for now ... */
                        if (value == 0x800 ||
                            value == 0xBC2)
                        {
                            ok = 0;
                        }
                        break;
                    }
                        
                    default:
                    {
                        if (value != 0x800 &&   /* not 1080p? */
                            value != 0xBC2)     /* not 720p? */
                        {
                            ok = 0;
                        }
                        break;
                    }
                }
            }
        }
        
        if (is_basic && !is_6D && !is_100D && !is_EOSM && !is_700D && !is_650D)
        {
            if (reg == 7)
            {
                found = 1;
                /* prevent running in 600D hack crop mode */
                if (value != 0x800)
                {
                    ok = 0;
                }
            }
        }
        
        data_buf++;
    }
    
    if (found) return ok;
    
    return -1;
}

/* pack two 6-bit values into a 12-bit one */
#define PACK12(lo,hi) ((((lo) & 0x3F) | ((hi) << 6)) & 0xFFF)

/* pack two 16-bit values into a 32-bit one */
#define PACK32(lo,hi) (((uint32_t)(lo) & 0xFFFF) | ((uint32_t)(hi) << 16))

/* pack two 16-bit values into a 32-bit one */
#define PACK32(lo,hi) (((uint32_t)(lo) & 0xFFFF) | ((uint32_t)(hi) << 16))

static void FAST cmos_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    /* make sure we are in 1080p/720p mode */
    if (!is_supported_mode())
    {
        /* looks like checking properties works fine for detecting
         * changes in video mode, but not for detecting the zoom change */
        return;
    }
    
    /* also check CMOS registers; in zoom mode, we get different values
     * and this check is instant (no delays).
     *
     * on 5D3, the 640x480 acts like 1080p during standby,
     * so properties are our only option for that one.
     */
    
    uint16_t* data_buf = (uint16_t*) regs[0];
    int ret = check_cmos_vidmode(data_buf);
    
    if (ret >= 0)
    {
        cmos_vidmode_ok = ret;
    }
    
    if (ret != 1)
    {
        return;
    }
    
    int cmos_new[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    
    if (is_5D3)
    {
        switch (crop_preset)
        {
                /* 1:1 (3x) */
            case CROP_PRESET_3X:
                /* start/stop scanning line, very large increments */
                /* note: these are two values, 6 bit each, trial and error */
                cmos_new[1] = (is_720p())
                ? PACK12(13,10)     /* 720p,  almost centered */
                : PACK12(11,11);    /* 1080p, almost centered */
                
                cmos_new[2] = 0x10E;    /* read every column, centered crop */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_3X_TALL:
                cmos_new[1] =           /* vertical centering (trial and error) */
                (video_mode_fps == 24) ? PACK12(8,13)  :
                (video_mode_fps == 25) ? PACK12(8,12)  :
                (video_mode_fps == 30) ? PACK12(9,11)  :
                (video_mode_fps == 50) ? PACK12(12,10) :
                (video_mode_fps == 60) ? PACK12(13,10) :
                (uint32_t) -1 ;
                cmos_new[2] = 0x10E;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
                /* 3x3 binning in 720p */
                /* 1080p it's already 3x3, don't change it */
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_50p:
            case CROP_PRESET_3x3_1X_60p:
            case CROP_PRESET_3x3_1X_100D:
            case CROP_PRESET_3x3_1X_EOSM:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_3x3_1X_45p:
                if (is_720p())
                {
                    /* start/stop scanning line, very large increments */
                    cmos_new[1] =
                    ((crop_preset == CROP_PRESET_3x3_1X_48p || crop_preset == CROP_PRESET_3x3_1X_45p) && (x3crop == 0x0 && !crop_patch)) ? PACK12(3,15) :
                    ((x3crop == 0x1 || crop_patch) && crop_preset == CROP_PRESET_3x3_1X_50p)                  	 ? PACK12(14,11) :
                    ((x3crop == 0x1 || crop_patch) && crop_preset == CROP_PRESET_3x3_1X_60p)                  	 ? PACK12(15,11) :
                    ((x3crop == 0x1 || crop_patch) && (crop_preset == CROP_PRESET_3x3_1X_48p || crop_preset == CROP_PRESET_3x3_1X_45p)) ? PACK12(11,10) :
                    (video_mode_fps == 50 || crop_preset == CROP_PRESET_3x3_1X_50p)                  ? PACK12(4,14) :
                    (video_mode_fps == 60 || crop_preset == CROP_PRESET_3x3_1X_60p)                  ? PACK12(6,14) :
                    (uint32_t) -1 ;
                    
                    cmos_new[6] = 0x370;    /* pink highlights without this */
                    if (x3crop == 0x1 || crop_patch)
                    {
                        cmos_new[2] = 0x10E;    /* read every column, centered crop */
                        cmos_new[6] = 0x170;    /* pink highlights without this */
                    }
                    
                }
                break;
                
            case CROP_PRESET_3K:
                cmos_new[1] =           /* vertical centering (trial and error) */
                (video_mode_fps == 24) ? PACK12(8,12)  :
                (video_mode_fps == 25) ? PACK12(8,12)  :
                (video_mode_fps == 30) ? PACK12(9,11)  :
                (video_mode_fps == 50) ? PACK12(13,10) :
                (video_mode_fps == 60) ? PACK12(14,10) :    /* 13,10 has better centering, but overflows */
                (uint32_t) -1 ;
                cmos_new[2] = 0x0BE;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_UHD:
                cmos_new[1] =
                (video_mode_fps == 24) ? PACK12(4,9)  :
                (video_mode_fps == 25) ? PACK12(4,9)  :
                (video_mode_fps == 30) ? PACK12(5,8)  :
                (video_mode_fps == 50) ? PACK12(12,9) :
                (video_mode_fps == 60) ? PACK12(13,9) :
                (uint32_t) -1 ;
                cmos_new[2] = 0x08E;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_4K_HFPS:
                cmos_new[1] =
                (video_mode_fps == 24) ? PACK12(4,15)  :
                (video_mode_fps == 25) ? PACK12(4,15)  :
                (video_mode_fps == 30) ? PACK12(6,14)  :
                (video_mode_fps == 50) ? PACK12(10,11) :
                (video_mode_fps == 60) ? PACK12(12,11) :
                (uint32_t) -1 ;
                cmos_new[2] = 0x07E;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_FULLRES_LV:
                cmos_new[1] = 0x800;    /* from photo mode */
                cmos_new[2] = 0x00E;    /* 8 in photo mode; E enables shutter speed control from ADTG 805E */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
                /* 1x3 binning (read every line, bin every 3 columns) */
            case CROP_PRESET_1x3:
                /* start/stop scanning line, very large increments */
                cmos_new[1] = 0x280;
                /* 1920x2400 */
                /* cmos_new[1] = 0x2a0; */
                
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
                /* 1x3 binning (read every line, bin every 3 columns) */
            case CROP_PRESET_1x3_17fps:
                /* start/stop scanning line, very large increments */
                cmos_new[1] = 0x380;
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
                
                /* 3x1 binning (bin every 3 lines, read every column) */
            case CROP_PRESET_3x1:
                cmos_new[2] = 0x10E;    /* read every column, centered crop */
                break;
                
                /* raw buffer centered in zoom mode */
            case CROP_PRESET_CENTER_Z:
                cmos_new[1] = PACK12(9+2,42+1); /* vertical (first|last) */
                cmos_new[2] = 0x09E;            /* horizontal offset (mask 0xFF0) */
                break;
        }
    }
    
    if (is_basic || is_100D || is_EOSM || is_700D || is_650D || is_6D)
    {
        switch (crop_preset)
        {
            case CROP_PRESET_mv1080p_mv720p_100D:
                cmos_new[8] = 0x400;
                break;
            case CROP_PRESET_3xcropmode_100D:
                cmos_new[5] = 0x380;            /* vertical (first|last) */
                cmos_new[7] = 0xb;
                break;
                
            case CROP_PRESET_2K_100D:
                cmos_new[7] = 0xaa9;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_3K_100D:
                cmos_new[5] = 0x280;             /* vertical (first|last) */
                cmos_new[7] = 0xaa9;            /* horizontal offset (mask 0xFF0) */
                break;
                
            case CROP_PRESET_4K_100D:
                cmos_new[5] = 0x200;            /* vertical (first|last) */
                cmos_new[7] = 0xf20;
                if (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9)
                {
                    cmos_new[5] = 0x80;
                }
                break;
                
            case CROP_PRESET_4K_3x1_100D:
                cmos_new[5] = 0x200;            /* vertical (first|last) */
                cmos_new[7] = 0xf20;
                break;
                
            case CROP_PRESET_5K_3x1_100D:
                cmos_new[5] = 0x0;            /* vertical (first|last) */
                cmos_new[7] = 0x6+ delta_head4;
                break;
                
            case CROP_PRESET_anamorphic_rewired_100D:
                cmos_new[5] = 0x20;
                cmos_new[7] = 0x200;
                break;
                
            case CROP_PRESET_3x3_1X_100D:
            case CROP_PRESET_3x3_1X_EOSM:
                /* start/stop scanning line, very large increments */
                cmos_new[7] = (is_6D) ? PACK12(37,10) : PACK12(6,29);
                break;
                
                /* raw buffer centered in zoom mode */
            case CROP_PRESET_CENTER_Z_EOSM:
                cmos_new[5] = 0x300;             /* vertical (first|last) */
                cmos_new[7] = 0xa49;            /* horizontal offset (mask 0xFF0) */
                break;
                
            case CROP_PRESET_2K_EOSM:
                cmos_new[7] = 0xaa9;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_3K_EOSM:
                cmos_new[5] = 0x280;             /* vertical (first|last) */
                cmos_new[7] = 0xaa9;            /* horizontal offset (mask 0xFF0) */
                if (ratios == 0x3)
                {
                    cmos_new[5] = 0x200;            /* vertical (first|last) */
                    cmos_new[7] = 0xf20;
                }
                break;
                
            case CROP_PRESET_4K_EOSM:
                cmos_new[5] = 0x200;            /* vertical (first|last) */
                cmos_new[7] = 0xf20;
                if (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9)
                {
                    cmos_new[5] = 0x80;
                }
                break;
                
            case CROP_PRESET_4K_3x1_EOSM:
                cmos_new[5] = 0x200;            /* vertical (first|last) */
                cmos_new[7] = 0xf20;
                break;
                
            case CROP_PRESET_5K_3x1_EOSM:
                cmos_new[5] = 0x0;            /* vertical (first|last) */
                cmos_new[7] = 0x6;
                break;
                
            case CROP_PRESET_4K_5x1_EOSM:
                cmos_new[5] = 0x280;            /* vertical (first|last) */
                break;
                
            case CROP_PRESET_3x3_mv1080_EOSM:
                if (x3crop == 0x1 || crop_patch)
                {
                    cmos_new[5] = 0x400;
                    cmos_new[7] = 0x647;
                }
                break;
                
            case CROP_PRESET_mcm_mv1080_EOSM:
                cmos_new[5] = 0x20;
                cmos_new[7] = 0x800;
                if (x3crop == 0x1 || crop_patch)
                {
                    cmos_new[5] = 0x380;
                    cmos_new[7] = 0xa6a;
                }
                if (ratios == 0x0 && (x3crop == 0x1 || crop_patch))
                {
                    cmos_new[5] = 0x380;
                    cmos_new[7] = 0x80b;
                }
                break;
                
            case CROP_PRESET_3x3_mv1080_48fps_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_700D:
            case CROP_PRESET_3x3_mv1080_48fps_650D:
                if (x3crop == 0x1 || crop_patch)
                {
                    cmos_new[5] = 0x400;
                    cmos_new[7] = 0x649;
                }
                break;
                
            case CROP_PRESET_anamorphic_rewired_EOSM:
                cmos_new[5] = 0x20;
                cmos_new[7] = 0x2c4;
                break;
                
            case CROP_PRESET_anamorphic_EOSM:
            case CROP_PRESET_anamorphic_700D:
            case CROP_PRESET_anamorphic_650D:
                cmos_new[5] = 0x20;
                cmos_new[7] = 0x2c4;
                break;
                
            case CROP_PRESET_x10_EOSM:
                // we are already in x5zoom so already set
                // cmos_new[5] = 0x300;
                if (x3crop == 0x0)
                {
                    cmos_new[7] = 0xa49;
                    if (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_EOSM) cmos_new[7] = 0xa49 - 102;
                    if (CROP_PRESET_MENU == CROP_PRESET_3x1_mv720_50fps_EOSM) cmos_new[7] = 0xa49 - 98;
                    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM) cmos_new[7] = 0xa49 - 100;
                    if (CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM) cmos_new[7] = 0xa49 - 98;
                    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_EOSM) cmos_new[7] = 0xa49 - 98;
                    if (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_EOSM) cmos_new[7] = 0xa49 - 102;
                    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D) cmos_new[7] = 0xa49 - 102;
                }
                if ((isoauto == 0x1 || isoauto == 0x2 || isoauto == 0x3) && lens_info.raw_iso == 0x0 && HDR_iso_a == 0x0)
                {
                    if (isoauto == 0x1 && lens_info.raw_iso_auto > 0x54) cmos_new[0] = 0x84b; // stick to iso 400
                    if (isoauto == 0x2 && lens_info.raw_iso_auto > 0x5c) cmos_new[0] = 0x86f; // stick to iso 800
                    if (isoauto == 0x3 && lens_info.raw_iso_auto > 0x63) cmos_new[0] = 0x893; // stick to iso 1600
                }
                break;
                
            case CROP_PRESET_2520_1418_700D:
            case CROP_PRESET_2520_1418_650D:
                cmos_new[7] = 0xaa9;    /* pink highlights without this */
                break;
                
            case CROP_PRESET_3K_700D:
            case CROP_PRESET_3K_650D:
                cmos_new[5] = 0x280;             /* vertical (first|last) */
                cmos_new[7] = 0xa89;            /* horizontal offset (mask 0xFF0) */
                break;
               
            case CROP_PRESET_28K_650D:
                cmos_new[5] = 0x280;             /* vertical (first|last) */
                cmos_new[7] = 0xaa9;            /* horizontal offset (mask 0xFF0) */
                break;
                
            case CROP_PRESET_3540_700D:
            case CROP_PRESET_3540_650D:
                cmos_new[5] = 0x200;            /* vertical (first|last) */
                break;
                
            case CROP_PRESET_UHD_700D:
            case CROP_PRESET_UHD_650D:
                cmos_new[5] = 0x180;            /* vertical (first|last) */
                cmos_new[7] = 0x306;            /* horizontal offset (mask 0xFF0) */
                break;
                
            case CROP_PRESET_4K_700D:
            case CROP_PRESET_4K_650D:
                cmos_new[5] = 0x100;            /* vertical (first|last) */
                cmos_new[7] = 0xF84;            /* horizontal offset (mask 0xFF0) */
                break;
                
            case CROP_PRESET_FULLRES_LV_700D:
            case CROP_PRESET_FULLRES_LV_650D:
                cmos_new[5] = 0x000;            /* vertical (first|last) */
                cmos_new[7] = 0x800;            /* horizontal offset (mask 0xFF0) */
                break;
                
                /* raw buffer centered in zoom mode */
            case CROP_PRESET_CENTER_Z_700D:
            case CROP_PRESET_CENTER_Z_650D:
                cmos_new[5] = 0x300;             /* vertical (first|last) */
                cmos_new[7] = 0xa49;            /* horizontal offset (mask 0xFF0) */
                break;
                
            case CROP_PRESET_3x3_mv1080_700D:
            case CROP_PRESET_3x3_mv1080_650D:
                cmos_new[8] = 0x400;    /* 0x400 in mv1080 0x800 in mv720 */
                break;
                
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_50p:
            case CROP_PRESET_3x3_1X_60p:
                /* start/stop scanning line, very large increments */
                cmos_new[7] = (is_6D) ? PACK12(37,10) : PACK12(6,29);
                break;
                
            case CROP_PRESET_HD100_6D:
                cmos_new[6] = 0x450;
                cmos_new[7] = 0x268 + delta_head4;
                break;
                
            case CROP_PRESET_HD75_6D:
                cmos_new[6] = 0x400;
                cmos_new[7] = 0x3cd + delta_head4;
                break;
                
            case CROP_PRESET_HD50_6D:
                cmos_new[6] = 0x400;
                cmos_new[7] = 0x7 + delta_head4;
                break;
                
            case CROP_PRESET_HD2_6D:
                cmos_new[6] = 0xd0;
                cmos_new[7] = 0x267 + delta_head4;
                break;
                
            case CROP_PRESET_2K_6D:
                cmos_new[6] = 0xd0;
                cmos_new[7] = 0x268 + delta_head4;
                break;
                
            case CROP_PRESET_ULTRA_HD_6D:
                cmos_new[6] = 0xd0 + delta_head3;
                cmos_new[7] = 0x248 + delta_head4;
                break;
                
            case CROP_PRESET_4K_6D:
                //cmos_new[3] = 0x77;
                //cmos_new[6] = 0x0;
                //cmos_new[7] = 0xa + delta_head4;
                break;
                
            case CROP_PRESET_5K_6D:
                cmos_new[6] = 0x0;
                cmos_new[7] = 0xc+ delta_head4;
                break;
                
            case CROP_PRESET_FullRes_6D:
                cmos_new[6] = 0x0;
                cmos_new[7] = 0x5 + delta_head4;
                break;
                
            case CROP_PRESET_1x3_6D:
                cmos_new[6] = 0x469 + delta_head3;
                cmos_new[7] = 0x327 + delta_head4; //0xd , 0x261, 0x369 and 0x3cc works
                break;
                
            case CROP_PRESET_1x3_24FPS_6D:
                cmos_new[6] = 0x435 + delta_head3 ;
                cmos_new[7] = 0x327 + delta_head4 ;
                break;
                
        }
        
        // Disable for now. Causing a black sun(highlight issue)
        /* all presets */
       // if (is_EOSM)
       // {
       //     /* hot/cold pixels. Usually 0x2. 0x34 to be tested */
       //     cmos_new[4] = 0x34;
      // }
        
    }
    
    /* restrict max auto iso to 400/800/1600 instead of skyrocketing to 6400 */
    if ((isoauto == 0x1 || isoauto == 0x2 || isoauto == 0x3) && (lens_info.raw_iso == 0x0 && HDR_iso_a == 0x0))
    {
        /* dummy reg */
        if (isoauto == 0x1 && lens_info.raw_iso_auto > 0x54) EngDrvOutLV(0xC0F0b12c, 0x7);
        if (isoauto == 0x2 && lens_info.raw_iso_auto > 0x5c) EngDrvOutLV(0xC0F0b12c, 0x8);
        if (isoauto == 0x3 && lens_info.raw_iso_auto > 0x63) EngDrvOutLV(0xC0F0b12c, 0x9);
        
        if ((!is_5D3 && !is_6D) && isoauto == 0x1 && lens_info.raw_iso_auto > 0x54) cmos_new[0] = 0x84b; // stick to iso 400
        if ((!is_5D3 && !is_6D) && isoauto == 0x2 && lens_info.raw_iso_auto > 0x5c) cmos_new[0] = 0x86f; // stick to iso 800
        if ((!is_5D3 && !is_6D) && isoauto == 0x3 && lens_info.raw_iso_auto > 0x63) cmos_new[0] = 0x893; // stick to iso 1600
        
        if (is_5D3 && isoauto == 0x1 && lens_info.raw_iso_auto > 0x54) cmos_new[0] = 0x223; // stick to iso 400
        if (is_5D3 && isoauto == 0x2 && lens_info.raw_iso_auto > 0x5c) cmos_new[0] = 0x333; // stick to iso 800
        if (is_5D3 && isoauto == 0x3 && lens_info.raw_iso_auto > 0x63) cmos_new[0] = 0x443; // stick to iso 1600
        
        if (is_6D && isoauto == 0x1 && lens_info.raw_iso_auto > 0x54) cmos_new[0] = 0x22; // stick to iso 400
        if (is_6D && isoauto == 0x2 && lens_info.raw_iso_auto > 0x5c) cmos_new[0] = 0x33; // stick to iso 800
        if (is_6D && isoauto == 0x3 && lens_info.raw_iso_auto > 0x63) cmos_new[0] = 0x44; // stick to iso 1600
    }
    
    if ((isoauto == 0x1 || isoauto == 0x2 || isoauto == 0x3) && (lens_info.raw_iso == 0x0 && HDR_iso_a == 0x0))
    {
        if (isoauto == 0x1 && lens_info.raw_iso_auto < 0x54) EngDrvOutLV(0xC0F0b12c, 0x0);
        if (isoauto == 0x2 && lens_info.raw_iso_auto < 0x5c) EngDrvOutLV(0xC0F0b12c, 0x0);
        if (isoauto == 0x3 && lens_info.raw_iso_auto < 0x63) EngDrvOutLV(0xC0F0b12c, 0x0);
    }
    
    
    /* fast access to iso with press down button */
    if (!is_6D && isoclimb != 0x0 && HDR_iso_a == 0x0)
    {
        
        /* check if masc selected isoauto in canon menu ;) */
        if (lens_info.raw_iso == 0x0)
        {
            NotifyBox(3000, "Turn off autoiso if using iso climb");
        }
        
        if (x3toggle == 0x1 && is_EOSM)
        {
            NotifyBox(1000, "Use x3crop toggle SET or turn iso climb off");
        }
        isopatch = 1;
        if (isoclimb == 0x2)
        {
            EngDrvOutLV(0xC0F0b12c, 0x12);
            if (!is_5D3 && !is_6D) cmos_new[0] = 0x827;
            if (is_5D3) cmos_new[0] = 0x113;
        }
        else if (isoclimb == 0x3)
        {
            EngDrvOutLV(0xC0F0b12c, 0x13);
            if (!is_5D3 && !is_6D) cmos_new[0] = 0x84b;
            if (is_5D3) cmos_new[0] = 0x223;
        }
        else if (isoclimb == 0x4)
        {
            EngDrvOutLV(0xC0F0b12c, 0x14);
            if (!is_5D3 && !is_6D) cmos_new[0] = 0x86f;
            if (is_5D3) cmos_new[0] = 0x333;
        }
        else if (isoclimb == 0x5)
        {
            EngDrvOutLV(0xC0F0b12c, 0x15);
            if (!is_5D3 && !is_6D) cmos_new[0] = 0x893;
            if (is_5D3) cmos_new[0] = 0x443;
        }
        else if (isoclimb == 0x6)
        {
            EngDrvOutLV(0xC0F0b12c, 0x16);
            if (!is_5D3 && !is_6D) cmos_new[0] = 0x8b7;
            if (is_5D3) cmos_new[0] = 0x553;
        }
        if (isoclimb == 0x1)
        {
            EngDrvOutLV(0xC0F0b12c, 0x11);
            if (!is_5D3 && !is_6D) cmos_new[0] = 0x803;
            if (is_5D3) cmos_new[0] = 0x3;
        }
    }
    
    
    /* menu overrides */
    if (cmos1_lo || cmos1_hi)
    {
        cmos_new[1] = PACK12(cmos1_lo,cmos1_hi);
    }
    
    if (cmos2)
    {
        cmos_new[2] = cmos2;
    }
    
    if (cmos3)
    {
        cmos_new[3] = cmos3;
    }
    
    if (cmos4)
    {
        cmos_new[4] = cmos4;
    }
    
    if (cmos5)
    {
        cmos_new[5] = cmos5;
    }
    
    if (cmos6)
    {
        cmos_new[6] = cmos6;
    }
    
    if (cmos7)
    {
        cmos_new[7] = cmos7;
    }
    
    if (cmos8)
    {
        cmos_new[8] = cmos8;
    }
    
    if (cmos9)
    {
        cmos_new[9] = cmos9;
    }
    
    
    /* HDR workaround eosm */
    if (is_EOSM && RECORDING && HDR_iso_a != 0x0)
    {
        
        /* iso reg */
        cmos_new[0] = cmos0;
        
        uint32_t iso_a;
        uint32_t iso_b;
        
        if (HDR_iso_a == 0x1) iso_a = 0x803; /* 100 */
        if (HDR_iso_a == 0x2) iso_a = 0x827; /* 200 */
        if (HDR_iso_a == 0x3) iso_a = 0x84b; /* 400 */
        if (HDR_iso_a == 0x4) iso_a = 0x86f; /* 800 */
        if (HDR_iso_a == 0x5) iso_a = 0x893; /* 1600 */
        if (HDR_iso_a == 0x6) iso_a = 0x8b7; /* 3200 */
        
        if (HDR_iso_b == 0x1) iso_b = 0x803;
        if (HDR_iso_b == 0x2) iso_b = 0x827;
        if (HDR_iso_b == 0x3) iso_b = 0x84b;
        if (HDR_iso_b == 0x4) iso_b = 0x86f;
        if (HDR_iso_b == 0x5) iso_b = 0x893;
        if (HDR_iso_b == 0x6) iso_b = 0x8b7;
        
        if (cmos0 == iso_a)
        {
            cmos0 = iso_b;
        }
        else
        {
            cmos0 = iso_a;
        }
        
    }
    
    /* HDR previewing eosm */
    if (is_EOSM && !RECORDING && HDR_iso_a != 0x0)
    {
        
        /* iso reg */
        cmos_new[0] = cmos0;
        
        uint32_t iso_a;
        uint32_t iso_b;
        
        if (HDR_iso_a == 0x1) iso_a = 0x803; /* 100 */
        if (HDR_iso_a == 0x2) iso_a = 0x827; /* 200 */
        if (HDR_iso_a == 0x3) iso_a = 0x84b; /* 400 */
        if (HDR_iso_a == 0x4) iso_a = 0x86f; /* 800 */
        if (HDR_iso_a == 0x5) iso_a = 0x893; /* 1600 */
        if (HDR_iso_a == 0x6) iso_a = 0x8b7; /* 3200 */
        
        if (HDR_iso_b == 0x1) iso_b = 0x803;
        if (HDR_iso_b == 0x2) iso_b = 0x827;
        if (HDR_iso_b == 0x3) iso_b = 0x84b;
        if (HDR_iso_b == 0x4) iso_b = 0x86f;
        if (HDR_iso_b == 0x5) iso_b = 0x893;
        if (HDR_iso_b == 0x6) iso_b = 0x8b7;
        
        if (get_halfshutter_pressed())
        {
            cmos0 = iso_b;
        }
        else
        {
            cmos0 = iso_a;
        }
        
    }
    
    /* copy data into a buffer, to make the override temporary */
    /* that means: as soon as we stop executing the hooks, values are back to normal */
    static uint16_t copy[512];
    uint16_t* copy_end = &copy[COUNT(copy)];
    uint16_t* copy_ptr = copy;
    
    while (*data_buf != 0xFFFF)
    {
        *copy_ptr = *data_buf;
        
        int reg = (*data_buf) >> 12;
        if (cmos_new[reg] != -1)
        {
            *copy_ptr = (reg << 12) | cmos_new[reg];
            dbg_printf("CMOS[%x] = %x\n", reg, cmos_new[reg]);
        }
        
        data_buf++;
        copy_ptr++;
        if (copy_ptr > copy_end) while(1);
    }
    *copy_ptr = 0xFFFF;
    
    /* pass our modified register list to cmos_write */
    regs[0] = (uint32_t) copy;
}

static uint32_t nrzi_encode( uint32_t in_val )
{
    uint32_t out_val = 0;
    uint32_t old_bit = 0;
    for (int num = 0; num < 31; num++)
    {
        uint32_t bit = in_val & 1<<(30-num) ? 1 : 0;
        if (bit != old_bit)
            out_val |= (1 << (30-num));
        old_bit = bit;
    }
    return out_val;
}

static uint32_t nrzi_decode( uint32_t in_val )
{
    uint32_t val = 0;
    if (in_val & 0x8000)
        val |= 0x8000;
    for (int num = 0; num < 31; num++)
    {
        uint32_t old_bit = (val & 1<<(30-num+1)) >> 1;
        val |= old_bit ^ (in_val & 1<<(30-num));
    }
    return val;
}

static int FAST adtg_lookup(uint32_t* data_buf, int reg_needle)
{
    while(*data_buf != 0xFFFFFFFF)
    {
        int reg = (*data_buf) >> 16;
        if (reg == reg_needle)
        {
            return *(uint16_t*)data_buf;
        }
    }
    return -1;
}

/* adapted from fps_override_shutter_blanking in fps-engio.c */
static int adjust_shutter_blanking(int old)
{
    /* sensor duty cycle: range 0 ... timer B */
    int current_blanking = nrzi_decode(old);
    
    int video_mode = get_video_mode_index();
    
    /* what value Canon firmware assumes for timer B? */
    int fps_timer_b_orig = default_timerB[video_mode];
    
    int current_exposure = fps_timer_b_orig - current_blanking;
    
    /* wrong assumptions? */
    if (current_exposure < 0)
    {
        return old;
    }
    
    int default_fps = default_fps_1k[video_mode];
    int current_fps = fps_get_current_x1000();
    
    dbg_printf("FPS %d->%d\n", default_fps, current_fps);
    
    float frame_duration_orig = 1000.0 / default_fps;
    float frame_duration_current = 1000.0 / current_fps;
    
    float orig_shutter = frame_duration_orig * current_exposure / fps_timer_b_orig;
    
    float new_shutter =
    (shutter_range == 0) ?
    ({
        /* original shutter speed from the altered video mode */
        orig_shutter;
    }) :
    ({
        /* map the available range of 1/4000...1/30 (24-30p) or 1/4000...1/60 (50-60p)
         * from minimum allowed (1/15000 with full-res LV) to 1/fps */
        int max_fps_shutter = (video_mode_fps <= 30) ? 33333 : 64000;
        int default_fps_adj = 1e9 / (1e9 / max_fps_shutter - 250);
        (orig_shutter - 250e-6) * default_fps_adj / current_fps;
    });
    
    /* what value is actually used for timer B? (possibly after our overrides) */
    int fps_timer_b = (shamem_read(0xC0F06014) & 0xFFFF) + 1;
    
    dbg_printf("Timer B %d->%d\n", fps_timer_b_orig, fps_timer_b);
    
    int new_exposure = new_shutter * fps_timer_b / frame_duration_current;
    int new_blanking = COERCE(fps_timer_b - new_exposure, 10, fps_timer_b - 2);
    
    dbg_printf("Exposure %d->%d (timer B units)\n", current_exposure, new_exposure);
    
#ifdef CROP_DEBUG
    float chk_shutter = frame_duration_current * new_exposure / fps_timer_b;
    dbg_printf("Shutter %d->%d us\n", (int)(orig_shutter*1e6), (int)(chk_shutter*1e6));
#endif
    
    dbg_printf("Blanking %d->%d\n", current_blanking, new_blanking);
    
    return nrzi_encode(new_blanking);
}

extern void fps_override_shutter_blanking();

static void FAST adtg_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    if (!is_supported_mode() || !cmos_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }
    
    if (!is_720p())
    {
        if (crop_preset == CROP_PRESET_3x3_1X ||
            crop_preset == CROP_PRESET_3x3_1X_50p ||
            crop_preset == CROP_PRESET_3x3_1X_60p ||
            crop_preset == CROP_PRESET_3x3_1X_100D ||
            crop_preset == CROP_PRESET_3x3_1X_45p ||
            crop_preset == CROP_PRESET_3x3_1X_48p)
        {
            /* these presets only have effect in 720p mode */
            return;
        }
    }
    
    /* This hook is called from the DebugMsg's in adtg_write,
     * so if we change the register list address, it won't be able to override them.
     * Workaround: let's call it here. */
    fps_override_shutter_blanking();
    
    uint32_t cs = regs[0];
    uint32_t *data_buf = (uint32_t *) regs[1];
    int dst = cs & 0xF;
    
    /* copy data into a buffer, to make the override temporary */
    /* that means: as soon as we stop executing the hooks, values are back to normal */
    static uint32_t copy[512];
    uint32_t* copy_end = &copy[COUNT(copy)];
    uint32_t* copy_ptr = copy;
    
    struct adtg_new
    {
        int dst;
        int reg;
        int val;
    };
    
    /* expand this as required */
    struct adtg_new adtg_new[30] = {{0}};
    
    const int blanking_reg_zoom   = (is_5D3) ? 0x805E : 0x805F;
    const int blanking_reg_nozoom = (is_5D3) ? 0x8060 : 0x8061;
    const int blanking_reg = (lv_dispsize == 1) ? blanking_reg_zoom : blanking_reg_nozoom;
    
    /* scan for shutter blanking and make both zoom and non-zoom value equal */
    /* (the values are different when using FPS override with ADTG shutter override) */
    /* (fixme: might be better to handle this in ML core?) */
    int shutter_blanking = 0;
    for (uint32_t * buf = data_buf; *buf != 0xFFFFFFFF; buf++)
    {
        int reg = (*buf) >> 16;
        if (reg == blanking_reg)
        {
            int val = (*buf) & 0xFFFF;
            shutter_blanking = val;
        }
    }
    
    /* some modes may need adjustments to maintain exposure */
    if (shutter_blanking)
    {
        
        if (!is_5D3 && !is_EOSM && !is_100D)
        {
            {
                shutter_blanking = adjust_shutter_blanking(shutter_blanking);
            }
        }
        
        if (is_5D3)
        {
            /* FIXME: remove this kind of hardcoded conditions */
            if ((crop_preset == CROP_PRESET_CENTER_Z && lv_dispsize != 1) ||
                (crop_preset != CROP_PRESET_CENTER_Z && lv_dispsize == 1))
            {
                shutter_blanking = adjust_shutter_blanking(shutter_blanking);
            }
        }
        
        if (is_100D)
        {
            if (slowshutter == 0x0 || (crop_preset != CROP_PRESET_4K_100D && slowshutter == 0x1)) shutter_blanking = adjust_shutter_blanking(shutter_blanking);
        }
        
        if (is_EOSM)
        {
            if (slowshutter == 0x0 && crop_preset == CROP_PRESET_4K_EOSM && timelapse != 0x0) shutter_blanking = adjust_shutter_blanking(shutter_blanking);
            if (crop_preset == CROP_PRESET_4K_EOSM && timelapse == 0x0) shutter_blanking = adjust_shutter_blanking(shutter_blanking);
            if (crop_preset != CROP_PRESET_4K_EOSM && crop_preset != CROP_PRESET_mcm_mv1080_EOSM) shutter_blanking = adjust_shutter_blanking(shutter_blanking);
        }
        
    }
    
    /* should probably be made generic */
    if (is_5D3 || is_100D || is_EOSM || is_700D || is_650D || is_6D)
    {
        adtg_new[20] = (struct adtg_new) {2, 0x8, 0x60 + reg_8};
        adtg_new[21] = (struct adtg_new) {2, 0x9, 0x61 + reg_9};
        adtg_new[22] = (struct adtg_new) {2, 0xA, 0x62 + reg_a};
        adtg_new[23] = (struct adtg_new) {2, 0xB, 0x60 + reg_b};
        adtg_new[24] = (struct adtg_new) {2, 0xFE, 04 + reg_fe};
        /* all modes may want to override shutter speed */
        /* ADTG[0x8060]: shutter blanking for 3x3 mode  */
        /* ADTG[0x805E]: shutter blanking for zoom mode  */
        adtg_new[0] = (struct adtg_new) {6, blanking_reg_zoom, shutter_blanking};
        adtg_new[1] = (struct adtg_new) {6, blanking_reg_nozoom, shutter_blanking};
        
        /* always disable Movie crop mode if using crop_rec presets, except for mcm mode, Only eosm and 100D */
        if (is_EOSM || is_100D)
        {
            /* always disable Movie crop mode if using crop_rec presets, except for mcm mode */
            if ((crop_preset == CROP_PRESET_mcm_mv1080_EOSM) || (crop_preset == CROP_PRESET_anamorphic_rewired_EOSM) || (crop_preset == CROP_PRESET_anamorphic_rewired_100D))
            {
                if (is_EOSM || is_100D) movie_crop_hack_enable();
            }
            else
            {
                if (is_EOSM || is_100D) movie_crop_hack_disable();
            }
            
        }
        /* Correct analog gain pushed autoiso wise otherwise. Only 14bit while recording or below applies */
        if (isoauto != 0x0 && bitdepth == 0x0 && (!is_5D3 && !is_6D))
        {
            
            //if (lens_info.raw_iso_auto > 0x54)
            //{
            ///* correct black level/tint when previewing */
            //EngDrvOutLV(0xc0f37aec, 0x73ca + reg_bl);
            //EngDrvOutLV(0xc0f37af8, 0x73ca + reg_bl);
            //EngDrvOutLV(0xc0f37b04, 0x73ca + reg_bl);
            //EngDrvOutLV(0xc0f37ae0, 0x73ca + reg_bl);
            //}
            
            /* true iso 400 */
            if (isoauto == 0x1 && lens_info.raw_iso_auto > 0x54)
            {
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 0x438 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 0x439 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 0x439 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 0x439 + reg_gain};
            }
            /* true iso 800 */
            if (isoauto == 0x2 && lens_info.raw_iso_auto > 0x5c)
            {
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 0x421 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 0x420 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 0x421 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 0x422 + reg_gain};
            }
            /* true iso 1600 */
            if (isoauto == 0x3 && lens_info.raw_iso_auto > 0x63)
            {
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 0x457 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 0x456 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 0x457 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 0x457 + reg_gain};
            }
            
        }
        
        /* only apply bit reducing while recording, not while idle */
        if ((RECORDING && (is_EOSM || is_100D || is_6D || is_5D3)) || (!is_EOSM && !is_100D && !is_6D && !is_5D3))
        {
            if (bitdepth == 0x1)
            {
                /* 8bit roundtrip only not applied here with following set ups */
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 12 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 12 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 12 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 12 + reg_gain};
            }
            
            if (bitdepth == 0x2)
            {
                /* 9bit roundtrip only not applied here with following set ups */
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 30 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 30 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 30 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 30 + reg_gain};
            }
            
            if (bitdepth == 0x3)
            {
                /* 10bit roundtrip only not applied here with following set ups */
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 60 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 60 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 60 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 60 + reg_gain};
            }
            
            if (bitdepth == 0x4)
            {
                /* 12bit roundtrip only not applied here with following set ups */
                adtg_new[13] = (struct adtg_new) {6, 0x8882, 250 + reg_gain};
                adtg_new[14] = (struct adtg_new) {6, 0x8884, 250 + reg_gain};
                adtg_new[15] = (struct adtg_new) {6, 0x8886, 250 + reg_gain};
                adtg_new[16] = (struct adtg_new) {6, 0x8888, 250 + reg_gain};
            }
            
        }
        
    }
    
    /* hopefully generic; to be tested later */
    if (1)
    {
        
        /* all presets */
        if (is_EOSM)
        {
            /* hot/cold pixels. Disables dancing pixels higher isos which suddenly sets it to 0x1. Thanks Levas */
            adtg_new[19] = (struct adtg_new) {6, 0x88b0, 0x0};
            /* unclear reg. Let´s keep it for later */
            // adtg_new[20] = (struct adtg_new) {6, 0x802c, 0x0};
            
        }
        
        switch (crop_preset)
        {
                /* all 1:1 modes (3x, 3K, 4K...) */
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3K:
            case CROP_PRESET_UHD:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_FULLRES_LV:
                /* ADTG2/4[0x8000] = 5 (set in one call) */
                /* ADTG2[0x8806] = 0x6088 on 5D3 (artifacts without it) */
                adtg_new[2] = (struct adtg_new) {6, 0x8000, 5};
                if (is_5D3) {
                    /* this register is model-specific */
                    adtg_new[3] = (struct adtg_new) {2, 0x8806, 0x6088};
                }
                break;
                
            case CROP_PRESET_3xcropmode_100D:
                adtg_new[22] = (struct adtg_new) {6, 0x8000, 5};
                break;
                
                /* 3x3 binning in 720p (in 1080p it's already 3x3) */
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_50p:
            case CROP_PRESET_3x3_1X_60p:
            case CROP_PRESET_3x3_1X_100D:
            case CROP_PRESET_3x3_1X_EOSM:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_3x3_1X_45p:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2 + reg_800c};
                if ((x3crop == 0x1 || crop_patch) && is_5D3)
                {
                    adtg_new[3] = (struct adtg_new) {6, 0x8000, 5 + reg_8000};
                    adtg_new[17] = (struct adtg_new) {2, 0x8806, 0x6088};
                }
                break;
                
                
            case CROP_PRESET_1x3:
            case CROP_PRESET_1x3_17fps:
                /* ADTG2/4[0x800C] = 0: read every line */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 0 + reg_800c};
                // adtg_new[3] = (struct adtg_new) {6, 0x8000, 6 + reg_8000};
                break;
                
            case CROP_PRESET_3x3_mv1080_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_700D:
            case CROP_PRESET_3x3_mv1080_48fps_650D:
            case CROP_PRESET_4K_3x1_EOSM:
            case CROP_PRESET_5K_3x1_EOSM:
            case CROP_PRESET_4K_3x1_100D:
            case CROP_PRESET_5K_3x1_100D:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2 + reg_800c};
                adtg_new[17] = (struct adtg_new) {6, 0x8183, 0x21 + reg_8183};
                adtg_new[18] = (struct adtg_new) {6, 0x8184, 0x7b + reg_8184};
                if (x3crop == 0x1 || crop_patch)
                {
                    adtg_new[2] = (struct adtg_new) {6, 0x800C, 0 + reg_800c};
                    adtg_new[3] = (struct adtg_new) {6, 0x8000, 5 + reg_8000};
                }
                break;
                
            case CROP_PRESET_mcm_mv1080_EOSM:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2 + reg_800c};
                adtg_new[3] = (struct adtg_new) {6, 0x8000, 6 + reg_8000};
                adtg_new[17] = (struct adtg_new) {6, 0x8183, 0x21 + reg_8183};
                adtg_new[18] = (struct adtg_new) {6, 0x8184, 0x7b + reg_8184};
                if (x3crop == 0x1 || crop_patch)
                {
                    adtg_new[2] = (struct adtg_new) {6, 0x800C, 0 + reg_800c};
                    adtg_new[3] = (struct adtg_new) {6, 0x8000, 5 + reg_8000};
                }
                break;
                
            case CROP_PRESET_3x1_mv720_50fps_EOSM:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 4 + reg_800c};
                adtg_new[3] = (struct adtg_new) {6, 0x8000, 6 + reg_8000};
                break;
                
            case CROP_PRESET_4K_5x1_EOSM:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 4 + reg_800c};
                adtg_new[3] = (struct adtg_new) {6, 0x8000, 6 + reg_8000};
                break;
                
            case CROP_PRESET_mv1080p_mv720p_100D:
                if (is_1080p())
                {
                    adtg_new[22] = (struct adtg_new) {6, 0x800C, 2 + reg_800c};
                }
                if (is_720p())
                {
                    adtg_new[22] = (struct adtg_new) {6, 0x800C, 4 + reg_800c};
                }
                break;
                
            case CROP_PRESET_anamorphic_rewired_EOSM:
            case CROP_PRESET_anamorphic_rewired_100D:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 0 + reg_800c};
                adtg_new[3] = (struct adtg_new) {6, 0x8000, 6};
                adtg_new[17] = (struct adtg_new) {6, 0x8183, 0x21 + reg_8183};
                adtg_new[18] = (struct adtg_new) {6, 0x8184, 0x7b + reg_8184};
                break;
                
            case CROP_PRESET_anamorphic_EOSM:
            case CROP_PRESET_anamorphic_700D:
            case CROP_PRESET_anamorphic_650D:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 0 + reg_800c};
                adtg_new[3] = (struct adtg_new) {6, 0x8000, 6};
                adtg_new[17] = (struct adtg_new) {6, 0x8183, 0x21 + reg_8183};
                adtg_new[18] = (struct adtg_new) {6, 0x8184, 0x7b + reg_8184};
                break;
                
            case CROP_PRESET_FULLRES_LV_700D:
            case CROP_PRESET_FULLRES_LV_650D:
                /* adjust vertical resolution */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 0};
                adtg_new[3] = (struct adtg_new) {6, 0x8000, 5};
                break;
                
            case CROP_PRESET_3x3_mv1080_700D:
            case CROP_PRESET_3x3_mv1080_650D:
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2};
                adtg_new[3] = (struct adtg_new) {6, 0x8172, 0x6fd};
                adtg_new[4] = (struct adtg_new) {6, 0x8173, 0x453};
                adtg_new[5] = (struct adtg_new) {6, 0x8178, 0x6fd};
                adtg_new[6] = (struct adtg_new) {6, 0x8179, 0x453};
                adtg_new[7] = (struct adtg_new) {6, 0x8196, 0x12e};
                adtg_new[8] = (struct adtg_new) {6, 0x8197, 0x38c};
                adtg_new[9] = (struct adtg_new) {6, 0x82b6, 0x6f4};
                break;
                
            case CROP_PRESET_2K_6D:
            case CROP_PRESET_ULTRA_HD_6D:
                //case CROP_PRESET_4K_6D:
            case CROP_PRESET_FullRes_6D:
            case CROP_PRESET_HD2_6D:
                adtg_new[18] = (struct adtg_new) {6, 0x8000, 5};
                adtg_new[19] = (struct adtg_new) {6, 0x800C, 0};
                
                break;
                
            case CROP_PRESET_3x3_1X_6D:
            case CROP_PRESET_HD100_6D:
            case CROP_PRESET_HD75_6D:
            case CROP_PRESET_HD50_6D:
                adtg_new[18] = (struct adtg_new) {6, 0x8000, 6};
                adtg_new[19] = (struct adtg_new) {6, 0x800C, 2};
                break;
                
            case CROP_PRESET_1x3_6D:
            case CROP_PRESET_1x3_24FPS_6D:
                adtg_new[18] = (struct adtg_new) {6, 0x8000, 6};
                adtg_new[19] = (struct adtg_new) {6, 0x800C, 0};
                adtg_new[20] = (struct adtg_new) {6, 0x8183, 0x35};
                adtg_new[21] = (struct adtg_new) {6, 0x8184, 0x3b};
                break;
                
            case CROP_PRESET_5K_6D:
                adtg_new[18] = (struct adtg_new) {6, 0x8000, 6};
                adtg_new[19] = (struct adtg_new) {6, 0x800C, 2};
                break;
                
                //for high ISO mode
            case CROP_PRESET_4K_6D:
                //adtg_new[18] = (struct adtg_new) {6, 0x8866, 0x3+delta_head3};
                adtg_new[19] = (struct adtg_new) {6, 0x88b0, 0x0};
                //adtg_new[20] = (struct adtg_new) {6, 0x8882, 0x1fff};
                //adtg_new[21] = (struct adtg_new) {6, 0x8884, 0x1fff};
                //adtg_new[22] = (struct adtg_new) {6, 0x8886, 0x1fff};
                //adtg_new[23] = (struct adtg_new) {6, 0x8888, 0x1fff};
                //adtg_new[24] = (struct adtg_new) {6, 0x7014, 0x12};
                break;
                
                /* 3x1 binning (bin every 3 lines, read every column) */
                /* doesn't work well, figure out why */
            case CROP_PRESET_3x1:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                /* ADTG2[0x8806] = 0x6088 on 5D3 (artifacts worse without it) */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2 + reg_800c};
                if (is_5D3) {
                    /* this register is model-specific */
                    adtg_new[3] = (struct adtg_new) {2, 0x8806, 0x6088};
                }
                break;
        }
    }
    
    /* these should work on all presets, on all DIGIC 5 models and also on recent DIGIC 4 */
    if ((1) && (CROP_PRESET_MENU != CROP_PRESET_mv1080p_mv720p_100D) && (CROP_PRESET_MENU != CROP_PRESET_3xcropmode_100D) && (CROP_PRESET_MENU != CROP_PRESET_3x3_mv1080_700D) && (CROP_PRESET_MENU != CROP_PRESET_3x3_mv1080_650D))
    {
        /* assuming FPS timer B was overridden before this */
        int fps_timer_b = (shamem_read(0xC0F06014) & (0xFFFF + reg_timing5));
        int readout_end = shamem_read(is_digic4 ? 0xC0F06088 : 0xC0F06804) >> 16;
        
        /* PowerSaveTiming registers */
        /* after readout is finished, we can turn off the sensor until the next frame */
        /* we could also set these to 0; it will work, but the sensor will run a bit hotter */
        /* to be tested to find out exactly how much */
        adtg_new[4]  = (struct adtg_new) {6, 0x8172, nrzi_encode(readout_end + 1 + reg_timing1) }; /* PowerSaveTiming ON (6D/700D) */
        adtg_new[5]  = (struct adtg_new) {6, 0x8178, nrzi_encode(readout_end + 1 + reg_timing1) }; /* PowerSaveTiming ON (5D3/6D/700D) */
        adtg_new[6]  = (struct adtg_new) {6, 0x8196, nrzi_encode(readout_end + 1 + reg_timing1) }; /* PowerSaveTiming ON (5D3) */
        
        adtg_new[7]  = (struct adtg_new) {6, 0x8173, nrzi_encode(fps_timer_b - 5 + reg_timing3) }; /* PowerSaveTiming OFF (6D/700D) */
        adtg_new[8]  = (struct adtg_new) {6, 0x8179, nrzi_encode(fps_timer_b - 5 + reg_timing2) }; /* PowerSaveTiming OFF (5D3/6D/700D) */
        adtg_new[9]  = (struct adtg_new) {6, 0x8197, nrzi_encode(fps_timer_b - 5 + reg_timing2) }; /* PowerSaveTiming OFF (5D3) */
        
        adtg_new[10] = (struct adtg_new) {6, 0x82B6, nrzi_encode(readout_end - 1 + reg_timing6) }; /* PowerSaveTiming ON? (700D); 2 units below the "ON" timing from above */
        
        /* ReadOutTiming registers */
        /* these shouldn't be 0, as they affect the image */
        adtg_new[11] = (struct adtg_new) {6, 0x82F8, nrzi_encode(readout_end + 1 + reg_timing4) }; /* ReadOutTiming */
        adtg_new[12] = (struct adtg_new) {6, 0x82F9, nrzi_encode(fps_timer_b - 1 + reg_timing4) }; /* ReadOutTiming end? */
    }
    
    while(*data_buf != 0xFFFFFFFF)
    {
        *copy_ptr = *data_buf;
        int reg = (*data_buf) >> 16;
        for (int i = 0; i < COUNT(adtg_new); i++)
        {
            if ((reg == adtg_new[i].reg) && (dst & adtg_new[i].dst))
            {
                int new_value = adtg_new[i].val;
                dbg_printf("ADTG%x[%x] = %x\n", dst, reg, new_value);
                *(uint16_t*)copy_ptr = new_value;
                
                if (reg == blanking_reg_zoom || reg == blanking_reg_nozoom)
                {
                    /* also override in original data structure */
                    /* to be picked up on the screen indicators */
                    *(uint16_t*)data_buf = new_value;
                }
            }
        }
        data_buf++;
        copy_ptr++;
        if (copy_ptr >= copy_end) while(1);
    }
    *copy_ptr = 0xFFFFFFFF;
    
    /* pass our modified register list to adtg_write */
    regs[1] = (uint32_t) copy;
}

/* changing bits */
static inline uint32_t reg_override_bits(uint32_t reg, uint32_t old_val)
{
    static int last_hs_unpress = 0;
    
    if (((zoomaid == 0x1 || zoomaid == 0x2) && !RECORDING && !is_6D && !is_5D3) &&
        (CROP_PRESET_MENU != CROP_PRESET_CENTER_Z_EOSM &&
         CROP_PRESET_MENU != CROP_PRESET_3x3_1X_EOSM &&
         CROP_PRESET_MENU != CROP_PRESET_2K_EOSM &&
         CROP_PRESET_MENU != CROP_PRESET_3K_EOSM &&
         CROP_PRESET_MENU != CROP_PRESET_4K_EOSM))
    {
        /* 100D is a buggy mf! */
        if (!get_halfshutter_pressed() && !is_100D) last_hs_unpress = get_ms_clock();
        
        /* x10crop preview hack */
        if (get_ms_clock() - last_hs_unpress > 200 && get_halfshutter_pressed())
        {
            /* checking passed 1500ms for when in canon menu. get_ms_clock() seems to be counting with no reset while in canon menu */
            if (!is_100D && (get_ms_clock() - last_hs_unpress < 1500)) crop_preset = CROP_PRESET_x10_EOSM;
            /* not working on 100D because of double refresh stuff */
            if (is_100D) crop_preset = CROP_PRESET_x10_EOSM;
        }
    }
    
    /* reset registry. Used for dummy check in mlv_lite.c when using realtime preview */
    if (!get_halfshutter_pressed() && (zoomaid == 0x1 || zoomaid == 0x2) && !RECORDING)
    {
        EngDrvOutLV(0xc0f11a88, 0x0);
    }
    
    /* only apply bit reducing while recording, not while idle */
    if ((RECORDING && (is_EOSM || is_100D || is_6D || is_5D3)) || (!is_EOSM && !is_100D && !is_6D && !is_5D3))
    {
        if (bitdepth == 0x1)
        {
            switch (reg)
            {
                    /* correct liveview brightness */
                case 0xC0F42744: return 0x6060606;
            }
        }
        
        if (bitdepth == 0x2)
        {
            switch (reg)
            {
                    /* correct liveview brightness */
                case 0xC0F42744: return 0x5050505;
            }
        }
        
        if (bitdepth == 0x3)
        {
            switch (reg)
            {
                    /* correct liveview brightness */
                case 0xC0F42744: return 0x4040404;
            }
        }
        
        if (bitdepth == 0x4)
        {
            switch (reg)
            {
                    /* correct liveview brightness */
                case 0xC0F42744: return 0x2020202;
            }
        }
    }
    
    
    if (RECORDING && bitdepth != 0x0 && (is_EOSM || is_100D))
    {
        /* correcting black level a bit. Compensating greenish tint. Only affects preview, not recordings */
        if (CROP_PRESET_MENU != CROP_PRESET_3x3_mv1080_48fps_EOSM)
        {
            if (lens_info.raw_iso != 0x48 && lens_info.raw_iso_auto > 0x4e) /* iso 100 excluded, breaks */
            {
                EngDrvOutLV(0xc0f37aec, 0x73ca + reg_bl);
                EngDrvOutLV(0xc0f37af8, 0x73ca + reg_bl);
                EngDrvOutLV(0xc0f37b04, 0x73ca + reg_bl);
                EngDrvOutLV(0xc0f37ae0, 0x73ca + reg_bl);
            }
            
        }
        
    }
    
    if (!RECORDING && (is_EOSM || is_100D || is_6D || is_5D3))
    {
        
        if (bitdepth == 0x1)
        {
            EngDrvOutLV(0xc0f0815c, 0x3);
        }
        
        if (bitdepth == 0x2)
        {
            EngDrvOutLV(0xc0f0815c, 0x4);
        }
        
        if (bitdepth == 0x3)
        {
            EngDrvOutLV(0xc0f0815c, 0x5);
        }
        
        if (bitdepth == 0x4)
        {
            EngDrvOutLV(0xc0f0815c, 0x6);
        }
        
        /* reset eosm switch */
        if (bitdepth == 0x0 && (is_EOSM || is_100D || is_6D || is_5D3))
        {
            
            if (timelapse != 0x0 && slowshutter == 0x1 && (crop_preset == CROP_PRESET_4K_100D || crop_preset == CROP_PRESET_4K_EOSM))
            {
                EngDrvOutLV(0xc0f0815c, 0x7);
            }
            else
            {
                EngDrvOutLV(0xc0f0815c, 0x2);
            }
            
        }
    }
    
    if (is_EOSM)
    {
        switch (reg)
        {
                /* not used but might be in the future */
            case 0xC0F06800: return 0x10010 + reg_6800_width + (reg_6800_height << 16);
        }
        
        /* HDR flag */
        if (HDR_iso_a != 0x0)
        {
            if (HDR_iso_a == 0x1) switch (reg) case 0xC0F0b12c: return 0x1;
            if (HDR_iso_a == 0x2) switch (reg) case 0xC0F0b12c: return 0x2;
            if (HDR_iso_a == 0x3) switch (reg) case 0xC0F0b12c: return 0x3;
            if (HDR_iso_a == 0x4) switch (reg) case 0xC0F0b12c: return 0x4;
            if (HDR_iso_a == 0x5) switch (reg) case 0xC0F0b12c: return 0x5;
            if (HDR_iso_a == 0x6) switch (reg) case 0xC0F0b12c: return 0x6;
        }
        
        if (HDR_iso_a == 0x0 && isoauto == 0x0 && isoclimb == 0x0)
        {
            EngDrvOutLV(0xC0F0b12c, 0x0);
        }
        
    }
    
    return 0;
}

/* this is used to cover the black bar at the top of the image in 1:1 modes */
/* (used in most other presets) */
static inline uint32_t reg_override_top_bar(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw start line/column */
            /* move start line down by 30 pixels */
            /* not sure where this offset comes from */
        case 0xC0F06800:
            return 0x1F0017;
    }
    
    return 0;
}

/* these are required for increasing vertical resolution */
/* (used in most other presets) */
static inline uint32_t reg_override_HEAD34(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* HEAD3 timer */
        case 0xC0F0713C:
            return old_val + YRES_DELTA + delta_head3;
            
            /* HEAD4 timer */
        case 0xC0F07150:
            return old_val + YRES_DELTA + delta_head4;
    }
    
    return 0;
}

static inline uint32_t reg_override_common(uint32_t reg, uint32_t old_val)
{
    uint32_t a = reg_override_top_bar(reg, old_val);
    if (a) return a;
    
    uint32_t b = reg_override_HEAD34(reg, old_val);
    if (b) return b;
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_fps(uint32_t reg, uint32_t timerA, uint32_t timerB, uint32_t old_val)
{
    /* hardware register requires timer-1 */
    timerA--;
    timerB--;
    
    /* only override FPS registers if the old value is what we expect
     * otherwise we may be in some different video mode for a short time
     * this race condition is enough to lock up LiveView in some cases
     * e.g. 5D3 3x3 50/60p when going from photo mode to video mode
     */
    
    switch (reg)
    {
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
        case 0xC0F06010:
        {
            uint32_t expected = default_timerA[get_video_mode_index()] - 1;
            
            if (old_val == expected || old_val == expected + 1)
            {
                return timerA;
            }
            
            break;
        }
            
        case 0xC0F06008:
        case 0xC0F0600C:
        {
            uint32_t expected = default_timerA[get_video_mode_index()] - 1;
            expected |= (expected << 16);
            
            if (old_val == expected || old_val == expected + 0x00010001)
            {
                return timerA | (timerA << 16);
            }
            
            break;
        }
            
        case 0xC0F06014:
        {
            uint32_t expected = default_timerB[get_video_mode_index()] - 1;
            
            if (old_val == expected || old_val == expected + 1)
            {
                return timerB;
            }
            
            break;
        }
    }
    
    return 0;
}

static inline uint32_t reg_override_3X_tall(uint32_t reg, uint32_t old_val)
{
    /* change FPS timers to increase vertical resolution */
    if (video_mode_fps >= 50)
    {
        int timerA = 400;
        
        int timerB =
        (video_mode_fps == 50) ? 1200 :
        (video_mode_fps == 60) ? 1001 :
        -1 ;
        
        int a = reg_override_fps(reg, timerA, timerB, old_val);
        if (a) return a;
    }
    
    /* fine-tuning head timers appears to help
     * pushing the resolution a tiny bit further */
    int head_adj =
    (video_mode_fps == 50) ? -30 :
    (video_mode_fps == 60) ? -20 :
    0 ;
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
        case 0xC0F06804:
            return old_val + (YRES_DELTA << 16);
            
            /* HEAD3 timer */
        case 0xC0F0713C:
            return old_val + YRES_DELTA + delta_head3 + head_adj;
            
            /* HEAD4 timer */
        case 0xC0F07150:
            return old_val + YRES_DELTA + delta_head4 + head_adj;
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3x3_50p(uint32_t reg, uint32_t old_val)
{
    if (!is_720p())
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }
    
    int timerA = 400;
    int timerB = 1200;
    
    /*  reduce to 30fps temporary to be able to go back from x10 zoom without freezes */
    if (lv_dispsize != 1) *(volatile uint32_t*)0xC0F06014 = 0x613;
    
    int a = reg_override_fps(reg, timerA, timerB, old_val);
    if (a) return a;
    
    switch (reg)
    {
            /* for some reason, top bar disappears with the common overrides */
            /* very tight fit - every pixel counts here */
        case 0xC0F06800:
            return 0x1D0017;
            
            /* raw resolution (end line/column) */
        case 0xC0F06804:
            return 0x3de011b + reg_6804_width + (reg_6804_height << 16);
            
            /* HEAD3 timer */
        case 0xC0F0713C:
            return 0x3de + reg_713c;
            
            /* HEAD4 timer */
        case 0xC0F07150:
            return 0x3d2 + reg_7150;
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3x3_60p(uint32_t reg, uint32_t old_val)
{
    if (!is_720p())
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }
    
    int timerA = 400;
    int timerB = 1001;
    
    /*  reduce to 30fps temporary to be able to go back from x10 zoom without freezes */
    if (lv_dispsize != 1) *(volatile uint32_t*)0xC0F06014 = 0x613;
    
    int a = reg_override_fps(reg, timerA, timerB, old_val);
    if (a) return a;
    
    switch (reg)
    {
            /* for some reason, top bar disappears with the common overrides */
            /* very tight fit - every pixel counts here */
        case 0xC0F06800:
            return 0x1D0017;
            
            /* raw resolution (end line/column) */
        case 0xC0F06804:
            return 0x343011b + reg_6804_width + (reg_6804_height << 16);
            
            /* HEAD3 timer */
        case 0xC0F0713C:
            return 0x32b + reg_713c;
            
            /* HEAD4 timer */
        case 0xC0F07150:
            return 0x2e5 + reg_7150;
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3x3_48p(uint32_t reg, uint32_t old_val)
{
    if (!is_720p())
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }
    
    /* change FPS timers to increase vertical resolution */
    if (video_mode_fps >= 50)
    {
        int timerA = 400;
        int timerB = 1250;
        
        /*  reduce to 30fps temporary to be able to go back from x10 zoom without freezes */
        if (lv_dispsize != 1) *(volatile uint32_t*)0xC0F06014 = 0x613;
        
        int a = reg_override_fps(reg, timerA, timerB, old_val);
        if (a) return a;
    }
    
    switch (reg)
    {
            /* for some reason, top bar disappears with the common overrides */
            /* very tight fit - every pixel counts here */
        case 0xC0F06800:
            return 0x1D0017;
            
            /* raw resolution (end line/column) */
        case 0xC0F06804:
            return 0x438011b + reg_6804_width + (reg_6804_height << 16);
            
            /* HEAD3 timer */
            /* 2E6 in 50p, 2B4 in 60p */
        case 0xC0F0713C:
            return 0x2a9 + reg_713c + YRES_DELTA + delta_head3;
            
            /* HEAD4 timer */
            /* 2B4 in 50p, 26D in 60p */
        case 0xC0F07150:
            return 0x263 + reg_7150 + YRES_DELTA + delta_head4;
            
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3x3_45p(uint32_t reg, uint32_t old_val)
{
    if (!is_720p())
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }
    
    /* change FPS timers to increase vertical resolution */
    if (video_mode_fps >= 50)
    {
        int timerA = 401;
        int timerB = 1330;
        
        /*  reduce to 30fps temporary to be able to go back from x10 zoom without freezes */
        if (lv_dispsize != 1) *(volatile uint32_t*)0xC0F06014 = 0x613;
        
        int a = reg_override_fps(reg, timerA, timerB, old_val);
        if (a) return a;
    }
    
    switch (reg)
    {
            /* for some reason, top bar disappears with the common overrides */
            /* very tight fit - every pixel counts here */
        case 0xC0F06800:
            return 0x1D0017;
            
            /* raw resolution (end line/column) */
        case 0xC0F06804:
            return old_val + (YRES_DELTA << 16);
            
            /* HEAD3 timer */
            /* 2E6 in 50p, 2B4 in 60p */
        case 0xC0F0713C:
            return 0x2A0 + YRES_DELTA + delta_head3;
            
            /* HEAD4 timer */
            /* 2B4 in 50p, 26D in 60p */
        case 0xC0F07150:
            return 0x22f + YRES_DELTA + delta_head4;
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3K(uint32_t reg, uint32_t old_val)
{
    /* FPS timer A, for increasing horizontal resolution */
    /* 25p uses 480 (OK), 24p uses 440 (too small); */
    /* only override in 24p, 30p and 60p modes */
    if (video_mode_fps != 25 && video_mode_fps !=  50)
    {
        int timerA = 455;
        int timerB =
        (video_mode_fps == 24) ? 2200 :
        (video_mode_fps == 30) ? 1760 :
        (video_mode_fps == 60) ?  880 :
        -1 ;
        
        int a = reg_override_fps(reg, timerA, timerB, old_val);
        if (a) return a;
    }
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804:
            return (old_val & 0xFFFF0000) + 0x1AA + (YRES_DELTA << 16);
            
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_4K_hfps(uint32_t reg, uint32_t old_val)
{
    /* FPS timer A, for increasing horizontal resolution */
    /* trial and error to allow 4096; 572 is too low, 576 looks fine */
    /* pick some values with small roundoff error */
    int timerA =
    (video_mode_fps < 30)  ?  585 : /* for 23.976/2 and 25/2 fps */
    579 ; /* for all others */
    
    /* FPS timer B, tuned to get half of the frame rate from Canon menu */
    int timerB =
    (video_mode_fps == 24) ? 3422 :
    (video_mode_fps == 25) ? 3282 :
    (video_mode_fps == 30) ? 2766 :
    (video_mode_fps == 50) ? 1658 :
    (video_mode_fps == 60) ? 1383 :
    -1 ;
    
    int a = reg_override_fps(reg, timerA, timerB, old_val);
    if (a) return a;
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (4096+140)/8 + 0x18, adjusted for 4096 in raw_rec */
        case 0xC0F06804:
            return (old_val & 0xFFFF0000) + 0x22A + (YRES_DELTA << 16);
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_UHD(uint32_t reg, uint32_t old_val)
{
    /* FPS timer A, for increasing horizontal resolution */
    /* trial and error to allow 3840; 536 is too low */
    int timerA =
    (video_mode_fps == 25) ? 547 :
    (video_mode_fps == 50) ? 546 :
    550 ;
    int timerB =
    (video_mode_fps == 24) ? 1820 :
    (video_mode_fps == 25) ? 1755 :
    (video_mode_fps == 30) ? 1456 :
    (video_mode_fps == 50) ?  879 :
    (video_mode_fps == 60) ?  728 :
    -1 ;
    
    int a = reg_override_fps(reg, timerA, timerB, old_val);
    if (a) return a;
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3840+140)/8 + 0x18, adjusted for 3840 in raw_rec */
        case 0xC0F06804:
            return (old_val & 0xFFFF0000) + 0x20A + (YRES_DELTA << 16);
    }
    
    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_fullres_lv(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06800:
            return 0x10018;         /* raw start line/column, from photo mode */
            
        case 0xC0F06804:            /* 1080p 0x528011B, photo 0xF6E02FE */
            return (old_val & 0xFFFF0000) + 0x2FE + (YRES_DELTA << 16);
            
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
            return 0x312;           /* from photo mode */
            
        case 0xC0F06010:            /* FPS timer A, for increasing horizontal resolution */
            return 0x317;           /* from photo mode; lower values give black border on the right */
            
        case 0xC0F06008:
        case 0xC0F0600C:
            return 0x3170317;
            
        case 0xC0F06014:
            return (video_mode_fps > 30 ? 856 : 1482) + YRES_DELTA;   /* up to 7.4 fps */
    }
    /* no need to adjust the black bar */
    return reg_override_HEAD34(reg, old_val);
}

/* just for testing */
/* (might be useful for FPS override on e.g. 70D) */
static inline uint32_t reg_override_40_fps(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
        case 0xC0F06010:
            return 0x18F;
            
        case 0xC0F06008:
        case 0xC0F0600C:
            return 0x18F018F;
            
        case 0xC0F06014:
            return 0x5DB;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_1x3(uint32_t reg, uint32_t old_val)
{
    
    switch (reg)
    {
        case 0xC0F06804:
            return set_25fps == 0x1 ? 0x8d6011b + reg_6804_width + (reg_6804_height << 16): 0x93a011b + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F0713c:
            return set_25fps == 0x1 ? 0x8da + reg_713c: 0x93e + reg_713c;
            
        case 0xC0F06014:
            return set_25fps == 0x1 ? 0x9bd + reg_6014: 0xa27 + reg_6014;
        case 0xC0F06008:
        case 0xC0F0600C:
            return 0x1800180 + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010:
            return 0x180 + reg_6008;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_1x3_17fps(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804:
            return 0xce6011b + reg_6804_width + (reg_6804_height << 16); /* 1920x3240(perfect 1920x1080) */
            
        case 0xC0F0713c:
            return 0xce6 + reg_713c;
            
        case 0xC0F06014:
            return 0xe51 + reg_6014;
        case 0xC0F06008:
        case 0xC0F0600C:
            return 0x1800180 + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010:
            return 0x180 + reg_6008;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_mv1080_mv720p(uint32_t reg, uint32_t old_val)
{
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_fps_nocheck(uint32_t reg, uint32_t timerA, uint32_t timerB, uint32_t old_val)
{
    /* hardware register requires timer-1 */
    timerA--;
    timerB--;
    
    switch (reg)
    {
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
        case 0xC0F06010:
        {
            return timerA;
        }
            
        case 0xC0F06008:
        case 0xC0F0600C:
        {
            return timerA | (timerA << 16);
        }
            
        case 0xC0F06014:
        {
            return timerB;
        }
    }
    
    return 0;
}

/* Values for 100D */
static inline uint32_t reg_override_3xcropmode_100d(uint32_t reg, uint32_t old_val)
{
    if (is_1080p())
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x4a701d7 + reg_6804_width + (reg_6804_height << 16);
        }
        
    }
    
    if (is_720p())
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x2d801d7 + reg_6804_width + (reg_6804_height << 16);
        }
        
    }
    
    switch (reg)
    {
        case 0xC0F06014: return set_25fps == 0x1 ? 0x89e + reg_6014: 0x8a1 + reg_6014;
        case 0xC0F0600c: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010: return set_25fps == 0x1 ? 0x25b - 24 + reg_6008: 0x25b + reg_6008;
    }
    
    return reg_override_bits(reg, old_val);
}

/* Values for 100D */
static inline uint32_t reg_override_2K_100d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x5ac02a1 + reg_6804_width + (reg_6804_height << 16); // 2520x1418  x5 Mode;
            
        case 0xC0F06014: return set_25fps == 0x1 ? 0x6d5 + reg_6014: 0x71e + reg_6014;
        case 0xC0F0713c: return 0x5ac + reg_713c;
        case 0xC0F07150: return 0x58c + reg_7150;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3K_100d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x5b90319 + reg_6804_width + (reg_6804_height << 16); // 3000x1432 24fps x5 Mode;
            
        case 0xC0F06824: return 0x3ca;
        case 0xC0F06828: return 0x3ca;
        case 0xC0F0682C: return 0x3ca;
        case 0xC0F06830: return 0x3ca;
            
        case 0xC0F06010: return 0x34b + reg_6008;
        case 0xC0F06008: return 0x34b034b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x34b034b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return 0x62c + reg_6014;
        case 0xC0F0713c: return 0x5ba + reg_713c;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_100d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x0) ? 0xbd904ff + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x1) ? 0x83504ff + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x2) ? 0x85904ff + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x3) ? 0xaff04ff + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x0) ? 0xbd90427 + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x1) ? 0x6cb0427 + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x2) ? 0x6e90427 + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x3) ? 0x9170427 + reg_6804_width + (reg_6804_height << 16):
            (ratios == 0x1) ? 0x6cb0427: (ratios == 0x2) ? 0x6e90427: (ratios == 0x3) ? 0x9170427: 0xbd90427;
            
        case 0xC0F06824: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
        case 0xC0F06828: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
        case 0xC0F0682C: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
        case 0xC0F06830: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
            
        case 0xC0F06010: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x57b: 0x45b + reg_6008;
        case 0xC0F06008: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x57b057b + reg_6008 + (reg_6008 << 16): 0x45b045b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x57b057b + reg_6008 + (reg_6008 << 16): 0x45b045b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return (RECORDING && timelapse == 0x1) ? 0xffff: (RECORDING && timelapse == 0x7) ? 0xffff:
            (RECORDING && timelapse == 0x2) ? 0x6ff9: (RECORDING && timelapse == 0x8) ? 0x58f7:
            (RECORDING && timelapse == 0x3) ? 0x37ff: (RECORDING && timelapse == 0x9) ? 0x2c7f:
            (RECORDING && timelapse == 0x4) ? 0x2553:
            (RECORDING && timelapse == 0x5) ? 0x1bfe:
            (RECORDING && timelapse == 0x6) ? 0x1665:
            (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0xcb7 + reg_6014: 0xfff + reg_6014;
            
            
        case 0xC0F0713c: return ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x0) ? 0xbd9 + reg_713c :
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x1) ? 0x835 + reg_713c :
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x2) ? 0x859 + reg_713c :
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x3) ? 0xaff + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x0) ? 0xbd9 + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x1) ? 0x6cb + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x2) ? 0x6e9 + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x3) ? 0x917 + reg_713c :
            (ratios == 0x1) ? 0x6cb: (ratios == 0x2) ? 0x6e9: (ratios == 0x3) ? 0x917: 0xbd9;
    }
    
    /* 4k timelapse function. For slowshutter this gives no first dark frame */
    if (RECORDING && timelapse == 0x1 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0xffff;
    if (RECORDING && timelapse == 0x2 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x6ff9;
    if (RECORDING && timelapse == 0x3 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x37ff;
    if (RECORDING && timelapse == 0x4 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x2553;
    if (RECORDING && timelapse == 0x5 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x1bfe;
    if (RECORDING && timelapse == 0x6 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x1665;
    
    /* 4k timelapse function. For slowshutter this gives no first dark frame */
    if (RECORDING && timelapse == 0x7 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0xffff;
    if (RECORDING && timelapse == 0x8 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x58f7;
    if (RECORDING && timelapse == 0x9 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x2c7f;
    
    /* set fps flag for mlv_lite.c */
    if (timelapse == 0x1) EngDrvOutLV(0xc0f0501c, 0x20);
    if (timelapse == 0x2) EngDrvOutLV(0xc0f0501c, 0x21);
    if (timelapse == 0x3) EngDrvOutLV(0xc0f0501c, 0x22);
    if (timelapse == 0x4) EngDrvOutLV(0xc0f0501c, 0x23);
    if (timelapse == 0x5) EngDrvOutLV(0xc0f0501c, 0x24);
    if (timelapse == 0x6) EngDrvOutLV(0xc0f0501c, 0x25);
    if (timelapse == 0x7) EngDrvOutLV(0xc0f0501c, 0x20);
    if (timelapse == 0x8) EngDrvOutLV(0xc0f0501c, 0x21);
    if (timelapse == 0x9) EngDrvOutLV(0xc0f0501c, 0x22);
    
    /* 4k timelapse function */
    if (!RECORDING && timelapse != 0x0 && slowshutter == 0x1)
    {
        switch (reg)
        {
            case 0xC0F42744: return (timelapse == 0x1 || timelapse == 0x7) ? 0x4040404:
                (timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x8) ? 0x3030303:
                (timelapse == 0x4 || timelapse == 0x5 || timelapse == 0x6 || timelapse == 0x9) ? 0x2020202: 0x1010101;
        }
    }
    
    /* resetting preview regs for 4k timelapse mode */
    if (RECORDING && timelapse != 0x0 && bitdepth == 0x0)
    {
        switch (reg)
        {
            case 0xC0F42744: return 0x1010101;
        }
        
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3x1_mv720_50fps_100d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x2d801d7 + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F0713c: return 0x305 + reg_713c;
        case 0xC0F07150: return 0x300 + reg_7150;
            
            /* 50 fps */
        case 0xC0F06014: return 0x4bb + reg_6014;
        case 0xC0F0600c: return 0x20f020f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x20f020f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010: return 0x20f + reg_6008;
            
        case 0xC0F06824: return 0x206 + reg_6824;
        case 0xC0F06828: return 0x206 + reg_6824;
        case 0xC0F0682c: return 0x206 + reg_6824;
        case 0xC0F06830: return 0x206 + reg_6824;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_3x1_100D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x3100413 + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F06824: return 0x4ca;
        case 0xC0F06828: return 0x4ca;
        case 0xC0F0682C: return 0x4ca;
        case 0xC0F06830: return 0x4ca;
        case 0xC0F06010: return 0x45f + reg_6008;
        case 0xC0F06008: return 0x45f050f + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x45f045f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06014: return 0x405 + reg_6014;
        case 0xC0F0713c: return 0x310 + reg_713c;
        case 0xC0F07150: return 0x305 + reg_7150;
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_5K_3x1_100D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x2e7050f + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F06824: return 0x56a;
        case 0xC0F06828: return 0x56a;
        case 0xC0F0682C: return 0x56a;
        case 0xC0F06830: return 0x56a;
        case 0xC0F06010: return 0x57b + reg_6008;
        case 0xC0F06008: return 0x57b057b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x57b057b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06014: return 0x3b5 + reg_6014;
        case 0xC0F0713c: return 0x2e8 + reg_713c;
        case 0xC0F07150: return 0x2e2 + reg_7150;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_1080p_100d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x45902a1 + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F0713c: return 0x459 + reg_713c;
        case 0xC0F07150: return 0x450 + reg_7150;
            
        case 0xC0F06014: return set_25fps == 0x1 ? 0x5ec + reg_6014: 0x62b + reg_6014;
        case 0xC0F06010: return 0x34b + reg_6008;
        case 0xC0F06008: return 0x34b034b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x34b034b + reg_6008 + (reg_6008 << 16);
            
    }
    
    return reg_override_bits(reg, old_val);
} 

static inline uint32_t reg_override_1x3_100d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x4c301d7 + reg_6804_width + (reg_6804_height << 16);
            
        case 0xC0F06014: return 0x9df + reg_6014;
        case 0xC0F0600c: return 0x20f020f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x20f020f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010: return 0x20f + reg_6008;
            
        case 0xC0F0713c: return 0x4c3 + reg_713c;
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_anamorphic_rewired_100d(uint32_t reg, uint32_t old_val)
{
    
    /* gets rid of the black border to the right. Connected to mlv_lite which takes over these regs while recording */
    if (!RECORDING)
    {
        EngDrvOutLV(0xc0f383d4, 0x4f0010 + reg_83d4);
        EngDrvOutLV(0xc0f383dc, 0x42401c6 + reg_83dc);
    }
    
    if ((ratios != 0x1) && (ratios != 0x2) && (ratios != 0x3))
    {
        
        switch (reg)
        {
            case 0xC0F06804: return 0x79f01ed + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return set_25fps == 0x1 ? 0x89e + reg_6014: 0x8a1 + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x25b - 24 + reg_6008: 0x25b + reg_6008;
                
            case 0xC0F0713c: return 0x79f + reg_713c;
            case 0xC0F07150: return 0x36c + reg_7150;
                
            case 0xC0F06824: return 0x722 + reg_6824;
            case 0xC0F06828: return 0x722 + reg_6824;
            case 0xC0F0682C: return 0x722 + reg_6824;
            case 0xC0F06830: return 0x722 + reg_6824;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
                
        }
        
    }
    
    
    if (ratios == 0x1 || ratios == 0x2)
    {
        /* 2.39:1 and 2.35:1 */
        switch (reg)
        {
            case 0xC0F06804: return 0x77701ed + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return set_25fps == 0x1 ? 0x89e + reg_6014: 0x8a1 + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x25b - 24 + reg_6008: 0x25b + reg_6008;
                
            case 0xC0F0713c: return 0x777 + reg_713c;
            case 0xC0F07150: return 0x36c + reg_7150;
                
            case 0xC0F06824: return 0x722 + reg_6824;
            case 0xC0F06828: return 0x722 + reg_6824;
            case 0xC0F0682C: return 0x722 + reg_6824;
            case 0xC0F06830: return 0x722 + reg_6824;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
                
        }
        
    }
    
    if (ratios == 0x3)
    {
        /* 16:9 */
        switch (reg)
        {
            case 0xC0F06804: return 0x79f01ed + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return set_25fps == 0x1 ? 0x89e + reg_6014: 0x8a1 + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x25b - 24 + reg_6008: 0x25b + reg_6008;
                
            case 0xC0F0713c: return 0x79f + reg_713c;
            case 0xC0F07150: return 0x36c + reg_7150;
                
            case 0xC0F06824: return 0x722 + reg_6824;
            case 0xC0F06828: return 0x722 + reg_6824;
            case 0xC0F0682C: return 0x722 + reg_6824;
            case 0xC0F06830: return 0x722 + reg_6824;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
        }
        
    }
    
    return reg_override_bits(reg, old_val);
}

/* Values for EOSM */
static inline uint32_t reg_override_center_z_eosm(uint32_t reg, uint32_t old_val)
{
    
    switch (reg)
    {
        case 0xC0F06804: return 0x4550298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
    }
    
    if (ratios == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x4550298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
        }
        
    }
    
    if (ratios == 0x2)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x4550298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
        }
        
    }
    
    /* fps and height window */
    switch (reg)
    {
        case 0xC0F0713c: return 0x455 + reg_713c;
        case 0xC0F07150: return 0x428 + reg_7150;
        case 0xC0F06014: return set_25fps == 0x1 ? 0x747 - 76 + reg_6014: 0x747 + reg_6014;
            /* reset dummy reg in raw.c */
        case 0xC0f0b13c: return 0xf;
    }
    
    return reg_override_bits(reg, old_val);
}

/* Values for EOSM */
static inline uint32_t reg_override_2K_eosm(uint32_t reg, uint32_t old_val)
{
    
    if (ratios == 0x1 || ratios == 0x2)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x44c0298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
                
            case 0xC0F0713c: return 0x44c + reg_713c;
            case 0xC0F07150: return 0x435 + reg_7150;
            case 0xC0F06014: return set_25fps == 0x1 ? 0x747 - 76 + reg_6014: 0x747 + reg_6014;
                
                /* reset dummy reg in raw.c */
            case 0xC0f0b13c: return 0xf;
        }
    }
    else
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x5a70298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1418  x5 Mode; */
            case 0xC0F0713c: return 0x5a7 + reg_713c;
            case 0xC0F07150: return 0x5a0 + reg_7150;
            case 0xC0F06014: return set_25fps == 0x1 ? 0x747 - 76 + reg_6014: 0x747 + reg_6014;
                
                /* reset dummy reg in raw.c */
            case 0xC0f0b13c: return 0xf;
        }
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3K_eosm(uint32_t reg, uint32_t old_val)
{
    
    if (ratios == 0x0)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x5b90318 + reg_6804_width + (reg_6804_height << 16); // 3032x1436  x5 Mode;
            case 0xC0F06014: return (get_halfshutter_pressed() && (zoomaid == 0x1 || zoomaid == 0x2) && !RECORDING) ? 0x839: 0x62c + reg_6014;
            case 0xC0F0713c: return 0x5b9 + reg_713c;
            case 0xC0F06824: return 0x3ca;
            case 0xC0F06828: return 0x3ca;
            case 0xC0F0682C: return 0x3ca;
            case 0xC0F06830: return 0x3ca;
            case 0xC0F06010: return 0x34b + reg_6008;
            case 0xC0F06008: return 0x34b034b + reg_6008 + (reg_6008 << 16);
            case 0xC0F0600C: return 0x34b034b + reg_6008 + (reg_6008 << 16);
                
                /* reset dummy reg in raw.c */
            case 0xC0f0b13c: return 0xf;
        }
    }
    
    if (ratios == 0x1 || ratios == 0x2)
    {
        switch (reg)
        {
                /* will change to 19fps for continous action */
            case 0xC0F06804: return 0x5190310 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x519 + reg_713c;
            case 0xC0F07150: return 0x514 + reg_7150;
            case 0xC0F06014: return 0x839 + reg_6014;
            case 0xC0F06824: return 0x3ca;
            case 0xC0F06828: return 0x3ca;
            case 0xC0F0682C: return 0x3ca;
            case 0xC0F06830: return 0x3ca;
            case 0xC0F06010: return 0x34b + reg_6008;
            case 0xC0F06008: return 0x34b034b + reg_6008 + (reg_6008 << 16);
            case 0xC0F0600C: return 0x34b034b + reg_6008 + (reg_6008 << 16);
        }
    }
    
    if (ratios == 0x3)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x6b50310 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x6b5 + reg_713c;
            case 0xC0F06824: return 0x4ca;
            case 0xC0F06828: return 0x4ca;
            case 0xC0F0682C: return 0x4ca;
            case 0xC0F06830: return 0x4ca;
            case 0xC0F06014: return 0xc71 + reg_6014;
            case 0xC0F06010: return 0x45b + reg_6008;
            case 0xC0F06008: return 0x45b045b + reg_6008 + (reg_6008 << 16);
            case 0xC0F0600C: return 0x45b045b + reg_6008 + (reg_6008 << 16);
        }
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_eosm(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x0) ? 0xbd704fe + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x1) ? 0x84104fe + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x2) ? 0x86504fe + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x3) ? 0xb0f04fe + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x0) ? 0xbd7041e + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x1) ? 0x6cb041e + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x2) ? 0x6e9041e + reg_6804_width + (reg_6804_height << 16):
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x3) ? 0x917041e + reg_6804_width + (reg_6804_height << 16):
            (ratios == 0x1) ? 0x6cb041e + reg_6804_width + (reg_6804_height << 16):
            (ratios == 0x2) ? 0x6e9041e + reg_6804_width + (reg_6804_height << 16):
            (ratios == 0x3) ? 0x917041e + reg_6804_width + (reg_6804_height << 16): 0xbd7041e + reg_6804_width + (reg_6804_height << 16);
            
        case 0xC0F06824: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
        case 0xC0F06828: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
        case 0xC0F0682C: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
        case 0xC0F06830: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56a: 0x4ca;
            
        case 0xC0F06010: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56b: 0x45b + reg_6008;
        case 0xC0F06008: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56b056b + reg_6008 + (reg_6008 << 16): 0x45b045b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0x56b056b + reg_6008 + (reg_6008 << 16): 0x45b045b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return ((RECORDING && timelapse == 0x1 && slowshutter == 0x1) || (timelapse == 0x1 && slowshutter == 0x0)) ? 0xffff:
            ((RECORDING && timelapse == 0x2 && slowshutter == 0x1) || (timelapse == 0x2 && slowshutter == 0x0)) ? 0x6ff9:
            ((RECORDING && timelapse == 0x3 && slowshutter == 0x1) || (timelapse == 0x3 && slowshutter == 0x0)) ? 0x37ff:
            ((RECORDING && timelapse == 0x4 && slowshutter == 0x1) || (timelapse == 0x4 && slowshutter == 0x0)) ? 0x2553:
            ((RECORDING && timelapse == 0x5 && slowshutter == 0x1) || (timelapse == 0x5 && slowshutter == 0x0)) ? 0x1bfe:
            ((RECORDING && timelapse == 0x6 && slowshutter == 0x1) || (timelapse == 0x6 && slowshutter == 0x0)) ? 0x1665:
            ((RECORDING && timelapse == 0x7 && slowshutter == 0x1) || (timelapse == 0x7 && slowshutter == 0x0)) ? 0xffff:
            ((RECORDING && timelapse == 0x8 && slowshutter == 0x1) || (timelapse == 0x8 && slowshutter == 0x0)) ? 0x58f7:
            ((RECORDING && timelapse == 0x9 && slowshutter == 0x1) || (timelapse == 0x9 && slowshutter == 0x0)) ? 0x2c7f:
            (timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) ? 0xcb7 + reg_6014:
            (ratios == 0x1 || ratios == 0x2) ? 0xc71 + reg_6014:
            (ratios == 0x0) ? 0x1665 + reg_6014: 0xfff + reg_6014;
            
        case 0xC0F0713c: return ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x0) ? 0xbd7 + reg_713c :
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x1) ? 0x841 + reg_713c :
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x2) ? 0x865 + reg_713c :
            ((timelapse == 0x7 || timelapse == 0x8 || timelapse == 0x9) && ratios == 0x3) ? 0xb0f + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x0) ? 0xbd7 + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x1) ? 0x6cb + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x2) ? 0x6e9 + reg_713c :
            ((timelapse == 0x1 || timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x4 || timelapse == 05 || timelapse == 0x6) && ratios == 0x3) ? 0x917 + reg_713c :
            (ratios == 0x1) ? 0x6cb: (ratios == 0x2) ? 0x6e9: (ratios == 0x3) ? 0x917: 0xbd7;
    }
    
    /* 4k timelapse function */
    if (RECORDING && timelapse == 0x1 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0xffff;
    if (RECORDING && timelapse == 0x2 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x6ff9;
    if (RECORDING && timelapse == 0x3 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x37ff;
    if (RECORDING && timelapse == 0x4 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x2553;
    if (RECORDING && timelapse == 0x5 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x1bfe;
    if (RECORDING && timelapse == 0x6 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x1665;
    
    /* 4k timelapse function. For slowshutter this gives no first dark frame */
    if (RECORDING && timelapse == 0x7 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0xffff;
    if (RECORDING && timelapse == 0x8 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x58f7;
    if (RECORDING && timelapse == 0x9 && slowshutter == 0x1) *(volatile uint32_t*)0xC0F06014 = 0x2c7f;
    
    /* set fps flag for mlv_lite.c */
    if (timelapse == 0x1 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x20);
    if (timelapse == 0x2 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x21);
    if (timelapse == 0x3 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x22);
    if (timelapse == 0x4 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x23);
    if (timelapse == 0x5 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x24);
    if (timelapse == 0x6 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x25);
    if (timelapse == 0x7 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x20);
    if (timelapse == 0x8 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x21);
    if (timelapse == 0x9 && slowshutter == 0x1) EngDrvOutLV(0xc0f0501c, 0x22);
    
    /* 4k timelapse function */
    if (!RECORDING && timelapse != 0x0 && slowshutter == 0x1)
    {
        switch (reg)
        {
            case 0xC0F42744: return (timelapse == 0x1 || timelapse == 0x7) ? 0x4040404:
                (timelapse == 0x2 || timelapse == 0x3 || timelapse == 0x8) ? 0x3030303:
                (timelapse == 0x4 || timelapse == 0x5 || timelapse == 0x6 || timelapse == 0x9) ? 0x2020202: 0x1010101;
        }
    }
    
    /* resetting preview regs for 4k timelapse mode */
    if (RECORDING && timelapse != 0x0 && bitdepth == 0x0 && slowshutter == 0x1)
    {
        switch (reg)
        {
            case 0xC0F42744: return 0x1010101;
        }
        
    }
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_3x1_EOSM(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x30f040a + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F06824: return 0x4ca;
        case 0xC0F06828: return 0x4ca;
        case 0xC0F0682C: return 0x4ca;
        case 0xC0F06830: return 0x4ca;
        case 0xC0F06010: return 0x45f + reg_6008;
        case 0xC0F06008: return 0x45f050f + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x45f045f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06014: return 0x405 + reg_6014;
        case 0xC0F0713c: return 0x320 + reg_713c;
        case 0xC0F07150: return 0x300 + reg_7150;
            
            /* reset dummy reg in raw.c */
        case 0xC0f0b13c: return 0xf;
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_5K_3x1_EOSM(uint32_t reg, uint32_t old_val)
{
    
    if (ratios == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x2e30504 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F06824: return 0x56a;
            case 0xC0F06828: return 0x56a;
            case 0xC0F0682C: return 0x56a;
            case 0xC0F06830: return 0x56a;
            case 0xC0F06010: return 0x57b + reg_6008;
            case 0xC0F06008: return 0x57b057b + reg_6008 + (reg_6008 << 16);
            case 0xC0F0600C: return 0x57b057b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06014: return 0x4b5 + reg_6014;
            case 0xC0F0713c: return 0x2e0 + reg_713c;
            case 0xC0F07150: return 0x299 + reg_7150;
                
                /* reset dummy reg in raw.c */
            case 0xC0f0b13c: return 0xf;
        }
    }
    else
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x2e50506 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F06824: return 0x56a;
            case 0xC0F06828: return 0x56a;
            case 0xC0F0682C: return 0x56a;
            case 0xC0F06830: return 0x56a;
            case 0xC0F06010: return 0x57b + reg_6008;
            case 0xC0F06008: return 0x57b057b + reg_6008 + (reg_6008 << 16);
            case 0xC0F0600C: return 0x57b057b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06014: return 0x3b5 + reg_6014;
            case 0xC0F0713c: return 0x2e4 + reg_713c;
            case 0xC0F07150: return 0x2ef + reg_7150;
                
                /* reset dummy reg in raw.c */
            case 0xC0f0b13c: return 0xf;
                
        }
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_5x1_EOSM(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06804: return 0x2d7040a + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F06824: return 0x4ca;
        case 0xC0F06828: return 0x4ca;
        case 0xC0F0682C: return 0x4ca;
        case 0xC0F06830: return 0x4ca;
        case 0xC0F06010: return 0x50f + reg_6008;
        case 0xC0F06008: return 0x50f050f + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x50f050f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06014: return 0x405 + reg_6014;
        case 0xC0F0713c: return 0x320 + reg_713c;
        case 0xC0F07150: return 0x300 + reg_7150;
            
            /* reset dummy reg in raw.c */
        case 0xC0f0b13c: return 0xf;
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3x3_mv1080_eosm(uint32_t reg, uint32_t old_val)
{
    
    /* 24 fps */
    if ((ratios == 0x1) || (ratios == 0x2) || (ratios == 0x3))
    {
        switch (reg)
        {
            case 0xC0F06014: return set_25fps == 0x1 ? 0xa03 - 103 + reg_6014: 0xa03 + reg_6014;
            case 0xC0F0600c: return 0x2070207 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return 0x2070207 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return 0x207 + reg_6008;
        }
    }
    
    switch (reg)
    {
        case 0xC0F06804: return 0x4a701d4 + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F0713c: return 0x4a7 + reg_713c;
        case 0xC0F07150: return 0x4a0 + reg_7150;
            
            /* dummy reg for height modes eosm in raw.c */
        case 0xC0f0b13c: return 0xa;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_mcm_mv1080_eosm(uint32_t reg, uint32_t old_val)
{
    
    /* gets rid of the black border to the right */
    EngDrvOutLV(0xc0f383d4, 0x4f0010 + reg_83d4);
    EngDrvOutLV(0xc0f383dc, 0x42401c6 + reg_83dc);
    
    if ((ratios == 0x0 && x3crop == 0x0) || (ratios == 0x0 && x3crop == 0x1))
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x4a601e4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x4a7 + reg_713c;
            case 0xC0F07150: return 0x430 + reg_7150;
        }
    }
    
    if ((ratios == 0x1 || ratios == 0x2 || ratios == 0x3) && x3crop == 0x0)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x42401e4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x424;
            case 0xC0F07150: return 0x38f;
                
                /* testing above for the sake of map files */
                // case 0xC0F06804: return 0x42401e4 + reg_6804_width + (reg_6804_height << 16);
                // case 0xC0F0713c: return 0x425 + reg_713c;
                // case 0xC0F07150: return 0x3ae + reg_7150;
        }
    }
    
    /* x3crop 2.39:1 and 2.35:1 */
    if ((ratios == 0x1 || ratios == 0x2) && x3crop == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x45601e4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x457 + reg_713c;
            case 0xC0F07150: return 0x3e0 + reg_7150;
        }
    }
    
    /* x3crop 16:9 */
    if (ratios == 0x3 && x3crop == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x45601e4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x457 + reg_713c;
            case 0xC0F07150: return 0x3e0 + reg_7150;
        }
    }
    
    
    if (set_25fps == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06014: return set_25fps == 0x1 ? 0x98c - 101 + reg_6014: 0x98c + reg_6014;
            case 0xC0F0600c: return 0x2210221 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return 0x2210221 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return 0x221 + reg_6008;
                
            case 0xC0F06824: return 0x21f + reg_6824;
            case 0xC0F06828: return 0x21f + reg_6824;
            case 0xC0F0682C: return 0x21f + reg_6824;
            case 0xC0F06830: return 0x21f + reg_6824;
        }
    }
    
    switch (reg)
    {
            /* reset dummy reg in raw.c */
        case 0xC0f0b13c: return 0xf;
            /* cinema cropmarks in mlv_lite.c. Detection reg */
        case 0xc0f0b134: return (ratios == 0x1 || ratios == 0x2) ? 0x5: 0x4;
    }
    
    
    /* 48fps preset 1496x838(16:9). Well, not really useful. Only framing preview, lagging. Let´s keep it for future work
     switch (reg)
     {         	case 0xC0F06804: return 0x36601a8 + reg_6804_width + (reg_6804_height << 16);
     case 0xC0F0713c: return 0x366 + reg_713c;
     case 0xC0F07150: return 0x300 + reg_7150;
     
     case 0xC0F06014: return 0x4fc + reg_6014;
     case 0xC0F0600c: return 0x2090209 + reg_6008 + (reg_6008 << 16);
     case 0xC0F06008: return 0x2090209 + reg_6008 + (reg_6008 << 16);
     case 0xC0F06010: return 0x209 + reg_6008;
     
     case 0xC0F06824: return 0x206 + reg_6824;
     case 0xC0F06828: return 0x206 + reg_6824;
     case 0xC0F0682C: return 0x206 + reg_6824;
     case 0xC0F06830: return 0x206 + reg_6824;
     }
     */
    
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3x3_48fps_eosm(uint32_t reg, uint32_t old_val)
{
    
    if (!is_720p() && !is_EOSM)
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }
    
    if ((ratios != 0x1) && (ratios != 0x2) && (ratios != 0x3))
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x3ef01d4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x3e1 + reg_713c;
            case 0xC0F07150: return 0x3dc + reg_7150;
                
                /* 46 fps */
            case 0xC0F06014: return 0x525 + reg_6014;
                
            case 0xC0F0600c: return 0x20f020f + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return 0x20f020f + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return 0x20f + reg_6008;
                
            case 0xC0F06824: return 0x206 + reg_6824;
            case 0xC0F06828: return 0x206 + reg_6824;
            case 0xC0F0682c: return 0x206 + reg_6824;
            case 0xC0F06830: return 0x206 + reg_6824;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xb;
        }
    }
    
    if (ratios == 0x1 || ratios == 0x2)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x2f701d4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x33d + reg_713c;
            case 0xC0F07150: return 0x2fa + reg_7150;
                
                /* 48/50fps. 50 if set_25fps is on */
            case 0xC0F06014: return set_25fps == 0x1 ? 0x4a1 + reg_6014: 0x4a2 + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x21b021b + reg_6008 + (reg_6008 << 16): 0x2310231 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x21b021b + reg_6008 + (reg_6008 << 16): 0x2310231 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x21b + reg_6008: 0x231 + reg_6008;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xe;
        }
    }
    
    if (ratios == 0x3)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x3ef01d4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c: return 0x3e3 + reg_713c;
            case 0xC0F07150: return 0x3dd + reg_7150;
                
                /* 46 fps */
            case 0xC0F06014: return 0x539 + reg_6014;
                
            case 0xC0F0600c: return 0x2130213 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return 0x2130213 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return 0x213 + reg_6008;
                
            case 0xC0F06824: return 0x206 + reg_6824;
            case 0xC0F06828: return 0x206 + reg_6824;
            case 0xC0F0682c: return 0x206 + reg_6824;
            case 0xC0F06830: return 0x206 + reg_6824;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xb;
        }
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3x1_mv720_50fps_eosm(uint32_t reg, uint32_t old_val)
{
    
    switch (reg)
    {
        case 0xC0F06804: return 0x2d701d4 + reg_6804_width + (reg_6804_height << 16);
        case 0xC0F0713c: return 0x2d7 + reg_713c;
        case 0xC0F07150: return 0x2d0 + reg_7150;
            
            /* 50 fps */
        case 0xC0F06014: return 0x4aa + reg_6014;
        case 0xC0F0600c: return 0x2170217 + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x2170217 + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010: return 0x217 + reg_6008;
            
        case 0xC0F06824: return 0x206 + reg_6824;
        case 0xC0F06828: return 0x206 + reg_6824;
        case 0xC0F0682c: return 0x206 + reg_6824;
        case 0xC0F06830: return 0x206 + reg_6824;
            
            /* reset dummy reg in raw.c */
        case 0xC0f0b13c: return 0xf;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_anamorphic_rewired_eosm(uint32_t reg, uint32_t old_val)
{
    
    /* gets rid of the black border to the right. Connected to mlv_lite which takes over these regs while recording */
    if (!RECORDING)
    {
        EngDrvOutLV(0xc0f383d4, 0x4f0010 + reg_83d4);
        EngDrvOutLV(0xc0f383dc, 0x42401c6 + reg_83dc);
    }
    
    if (ratios == 0x1 || ratios == 0x2)
    {
        switch (reg)
        {
                /* 2.39:1 or 2.35:1 */
            case 0xC0F06804:
                return bitdepth == 0x4 ? 0x6d701e4 + reg_6804_width + (reg_6804_height << 16): 0x79f01e4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c:
                return bitdepth == 0x4 ? 0x6d7 + reg_713c: 0x79f + reg_713c;
                
            case 0xC0F06824: return bitdepth == 0x4 ? 0x6d4 + reg_6824: 0x79d + reg_6824;
            case 0xC0F06828: return bitdepth == 0x4 ? 0x6d4 + reg_6824: 0x79d + reg_6824;
            case 0xC0F0682C: return bitdepth == 0x4 ? 0x6d4 + reg_6824: 0x79d + reg_6824;
            case 0xC0F06830: return bitdepth == 0x4 ? 0x6d4 + reg_6824: 0x79d + reg_6824;
                
            case 0xC0F06014: return set_25fps == 0x1 ? 0x89e + reg_6014: 0x8a1 + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x25b - 24 + reg_6008: 0x25b + reg_6008;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
                
        }
        
    }
    else
    {
        /* 16:9 */
        switch (reg)
        {
                
            case 0xC0F06804:
                return bitdepth == 0x4 ? 0x73b01e4 + reg_6804_width + (reg_6804_height << 16): 0x79f01e4 + reg_6804_width + (reg_6804_height << 16);
            case 0xC0F0713c:
                return bitdepth == 0x4 ? 0x73b + reg_713c: 0x79f + reg_713c;
                
            case 0xC0F06824: return bitdepth == 0x4 ? 0x738 + reg_6824: 0x79d + reg_6824;
            case 0xC0F06828: return bitdepth == 0x4 ? 0x738 + reg_6824: 0x79d + reg_6824;
            case 0xC0F0682C: return bitdepth == 0x4 ? 0x738 + reg_6824: 0x79d + reg_6824;
            case 0xC0F06830: return bitdepth == 0x4 ? 0x738 + reg_6824: 0x79d + reg_6824;
                
            case 0xC0F06014: return set_25fps == 0x1 ? 0x89e + reg_6014: 0x8a1 + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x25b025b - 24 + reg_6008 + (reg_6008 << 16): 0x25b025b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x25b - 24 + reg_6008: 0x25b + reg_6008;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
        }
        
    }
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_anamorphic_eosm(uint32_t reg, uint32_t old_val)
{
    
    if (ratios == 0x1 || ratios == 0x2)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x79f01d4 + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return 0x8ec + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x2470247 - 24 + reg_6008 + (reg_6008 << 16): 0x2470247 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x2470247 - 24 + reg_6008 + (reg_6008 << 16): 0x2470247 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x247 - 24 + reg_6008: 0x247 + reg_6008;
                
            case 0xC0F0713c: return 0x797 + reg_713c;
            case 0xC0F07150: return 0x791 + reg_7150;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
                
        }
        
    }
    
    if (ratios == 0x3)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x7ef01d4 + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return 0x95d + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x22b022b - 22 + reg_6008 + (reg_6008 << 16): 0x22b022b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x22b022b - 22 + reg_6008 + (reg_6008 << 16): 0x22b022b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x22b - 24 + reg_6008: 0x22b + reg_6008;
                
            case 0xC0F0713c: return 0x7ef + reg_713c;
            case 0xC0F07150: return 0x7dd + reg_7150;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
        }
        
    }
    
    if ((ratios != 0x1) && (ratios != 0x2) && (ratios != 0x3))
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x88501c2 + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return 0x99d + reg_6014;
            case 0xC0F0600c: return 0x21d021d + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return 0x21d021d + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return 0x21d + reg_6008;
                
            case 0xC0F0713c: return 0x885 + reg_713c;
            case 0xC0F07150: return 0x880 + reg_7150;
                
                /* dummy reg for height modes eosm in raw.c */
            case 0xC0f0b13c: return 0xd;
        }
        
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_x10_eosm(uint32_t reg, uint32_t old_val)
{
    
    /* dark mode */
    if (zoomaid == 0x2)
    {
        *(volatile uint32_t*)0xc0f06014 = 0xc71;
        *(volatile uint32_t*)0xc0f140c0 = 0xb0;
    }
    
    return 0;
}

static inline uint32_t reg_override_crop_preset_off_eosm(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* reset height modes eosm. For raw.c */
        case 0xC0f0b13c: return 0xf;
    }
    
    return 0;
}

static inline uint32_t reg_override_2520_700d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x5840298 + reg_6804_width + (reg_6804_height << 16); // 2520x1386  x5 Mode;
            
        case 0xC0F06014: return 0x747 + reg_6014;
            
        case 0xC0F0713c: return 0x5C7 + reg_713c;
        case 0xC0F07150: return 0x59A + reg_7150;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3K_700d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x5340322 + reg_6804_width + (reg_6804_height << 16); // 3072x1306  x5 Mode;
            
        case 0xC0F06824: return 0x3ca + reg_6824;
        case 0xC0F06828: return 0x3ca + reg_6824;
        case 0xC0F0682C: return 0x3ca + reg_6824;
        case 0xC0F06830: return 0x3ca + reg_6824;
            
        case 0xC0F06010: return 0x37b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x37b037b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x37b037b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return 0x6d7 + reg_6014;
            
        case 0xC0F0713c: return 0x555 + reg_713c;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3540(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x4540398 + reg_6804_width + (reg_6804_height << 16); // 3544x1080  x5 Mode;
            
        case 0xC0F06824: return 0x3ca + reg_6824;
        case 0xC0F06828: return 0x3ca + reg_6824;
        case 0xC0F0682C: return 0x3ca + reg_6824;
        case 0xC0F06830: return 0x3ca + reg_6824;
            
        case 0xC0F06010: return 0x3cb + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x3cb03cb + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x3cb03cb + reg_6008 + (reg_6008 << 16);
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_UHD_700d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x88d03e2 + reg_6804_width + (reg_6804_height << 16); // 3840x2160  x5 Mode;
            
        case 0xC0F06824: return 0x4ca + reg_6824;
        case 0xC0F06828: return 0x4ca + reg_6824;
        case 0xC0F0682C: return 0x4ca + reg_6824;
        case 0xC0F06830: return 0x4ca + reg_6824;
            
        case 0xC0F06010: return 0x43b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x43b043b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x43b043b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return 0x993 + reg_6014;
            
        case 0xC0F0713c: return 0x8E7 + reg_713c;
        case 0xC0F07150: return 0x8BA + reg_7150;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_700d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0xA1C0422 + reg_6804_width + (reg_6804_height << 16); // 4096x2560  x5 Mode;
            
        case 0xC0F06824: return 0x4ca + reg_6824;
        case 0xC0F06828: return 0x4ca + reg_6824;
        case 0xC0F0682C: return 0x4ca + reg_6824;
        case 0xC0F06830: return 0x4ca + reg_6824;
            
        case 0xC0F06010: return 0x45b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x45b045b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x45b045b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return 0xBD3 + reg_6014;
            
        case 0xC0F0713c: return 0xA55 + reg_713c;
    }
    
    return reg_override_bits(reg, old_val);
}


static inline uint32_t reg_override_fullres_lv_700d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0xcbc0538 + reg_6804_width + (reg_6804_height << 16); // 2520x1386  x5 Mode;
        case 0xC0F06800: return 0x30010;
            
        case 0xC0F06824: return 0x56a + reg_6824;
        case 0xC0F06828: return 0x56a + reg_6824;
        case 0xC0F0682C: return 0x56a + reg_6824;
        case 0xC0F06830: return 0x56a + reg_6824;
            
        case 0xC0F06010: return 0x56b + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x56b056b + reg_6008 + (reg_6008 << 16);
        case 0xC0F0600C: return 0x56b056b + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F06014: return 0xdcb + reg_6014;
            
        case 0xC0F0713c: return 0xca7 + reg_713c;
    }
    
    return reg_override_bits(reg, old_val);
}

/* Values for 700D */
static inline uint32_t reg_override_center_z_700d(uint32_t reg, uint32_t old_val)
{
    
    switch (reg)
    {
        case 0xC0F06804: return 0x4550298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
    }
    
    if (ratios == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x4550298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
        }
        
    }
    
    if (ratios == 0x2)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x4550298 + reg_6804_width + (reg_6804_height << 16); /* 2520x1072  x5 Mode; */
        }
        
    }
    
    /* fps and height window */
    switch (reg)
    {
        case 0xC0F0713c: return 0x455 + reg_713c;
        case 0xC0F07150: return 0x428 + reg_7150;
        case 0xC0F06014: return set_25fps == 0x1 ? 0x747 - 76 + reg_6014: 0x747 + reg_6014;
            /* reset dummy reg in raw.c */
        case 0xC0f0b13c: return 0xf;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_3x3_mv1080_700d(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x4a601d4 + reg_6804_width + (reg_6804_height << 16);
            
        case 0xC0F06014: return 0x7cf + reg_6014;
        case 0xC0F0600c: return 0x27f027f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06008: return 0x27f027f + reg_6008 + (reg_6008 << 16);
        case 0xC0F06010: return 0x27f + reg_6008 + (reg_6008 << 16);
            
        case 0xC0F37014: return 0xe;
            
        case 0xC0F0713c: return 0x4a7 + reg_713c;
        case 0xC0F07150: return 0x475 + reg_7150;
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_anamorphic_700d(uint32_t reg, uint32_t old_val)
{
    
    if (ratios == 0x1)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x79f01d4 + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return 0x8ec + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x2470247 - 24 + reg_6008 + (reg_6008 << 16): 0x2470247 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x2470247 - 24 + reg_6008 + (reg_6008 << 16): 0x2470247 + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x247 - 24 + reg_6008: 0x247 + reg_6008;
                
            case 0xC0F0713c: return 0x797 + reg_713c;
            case 0xC0F07150: return 0x791 + reg_7150;
        }
        
    }
    
    if (ratios == 0x2)
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x7ef01d4 + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return 0x95d + reg_6014;
            case 0xC0F0600c: return set_25fps == 0x1 ? 0x22b022b - 22 + reg_6008 + (reg_6008 << 16): 0x22b022b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return set_25fps == 0x1 ? 0x22b022b - 22 + reg_6008 + (reg_6008 << 16): 0x22b022b + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return set_25fps == 0x1 ? 0x22b - 24 + reg_6008: 0x22b + reg_6008;
                
            case 0xC0F0713c: return 0x7ef + reg_713c;
            case 0xC0F07150: return 0x7dd + reg_7150;
        }
        
    }
    
    if ((ratios != 0x1) && (ratios != 0x2))
    {
        switch (reg)
        {
            case 0xC0F06804: return 0x88501c2 + reg_6804_width + (reg_6804_height << 16);
                
            case 0xC0F06014: return 0x99d + reg_6014;
            case 0xC0F0600c: return 0x21d021d + reg_6008 + (reg_6008 << 16);
            case 0xC0F06008: return 0x21d021d + reg_6008 + (reg_6008 << 16);
            case 0xC0F06010: return 0x21d + reg_6008;
                
            case 0xC0F0713c: return 0x885 + reg_713c;
            case 0xC0F07150: return 0x880 + reg_7150;
        }
        
    }
    
    
    /* To be tested and refined
     switch (reg)
     {
     case 0xC0F06804: return 0x88501b2 + reg_6804_width + (reg_6804_height << 16);
     
     case 0xC0F06014: return 0x99d + reg_6014;
     case 0xC0F0600c: return 0x21d021d + reg_6008 + (reg_6008 << 16);
     case 0xC0F06008: return 0x21d021d + reg_6008 + (reg_6008 << 16);
     case 0xC0F06010: return 0x21d + reg_6008;
     
     case 0xC0F0713c: return 0x885 + reg_713c;
     case 0xC0F07150: return 0x880 + reg_7150;
     }
     */
    
    return reg_override_bits(reg, old_val);
}

/* Values for 6D */
static inline uint32_t reg_override_HD100_6D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* case 0xC0F06804: return 0x33f01ec; // 1832 x 869 6d; */
        case 0xC0F06804: return 0x1a601c8;
            
        case 0xC0F08184: return 0x1;
        case 0xC0F08188: return 0x1;
        case 0xC0F08194: return 0x1;
        case 0xC0F08198: return 0x1;
            
        case 0xC0F06824: return 0x1da;
        case 0xC0F06828: return 0x1da;
        case 0xC0F0682C: return 0x1da;
        case 0xC0F06830: return 0x1da;
            
        case 0xC0F06010: return 0x1dc;
        case 0xC0F06008: return 0x1dc01dc;
        case 0xC0F0600C: return 0x1dc01dc;
            
        case 0xC0F06014: return 0x217;
            /*case 0xC0F06014: return 0x3df; for 50fps in 720p mode*/
            /* case 0xC0F06014: return 0x561; for 36 fps */
            /* case 0xC0F06014: return 0x588; for 35fps */
            
        case 0xC0F0713c: return 0x1a7; // Same as 6804?; */
        case 0xC0F07150: return 0x151; // change with vertical res change - 6804->664 by default value for 7150 = 268 */
            /*case 0xC0F0713c: return 0x384;*/
            /*case 0xC0F07150: return 0x383;*/
            
    }
    
    return reg_override_bits(reg, old_val);
}

//1688x560 at 75fps
static inline uint32_t reg_override_HD75_6D(uint32_t reg, uint32_t old_val)
{
    
    //        EngDrvOutLV(0xc0f38024, 0x24c01bb+delta_head3); // fixes Canon liveview
    
    //EngDrvOutLV(0xc0f37a70, 0x24c0377);
    //EngDrvOutLV(0xc0f37b8c, 0x24c0377);
    //EngDrvOutLV(0xc0f37ad0, 0x24c0377);
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
        case 0xC0F06804: return 0x24e01c8;
            
        case 0xC0F08184: return 0x1;
        case 0xC0F08188: return 0x1;
        case 0xC0F08194: return 0x1;
        case 0xC0F08198: return 0x1;
            //        case 0xC0F081dc: return 0x378;
            
            
            //        case 0xC0F09008: return 0x24c0377+delta_head3;
            
            
        case 0xC0F06824: return 0x1da;
        case 0xC0F06828: return 0x1da;
        case 0xC0F0682C: return 0x1da;
        case 0xC0F06830: return 0x1da;
            
        case 0xC0F06010: return 0x1dc;
        case 0xC0F06008: return 0x1dc01dc;
        case 0xC0F0600C: return 0x1dc01dc;
            
        case 0xC0F06014: return 0x2cb;
            
        case 0xC0F0713c: return 0x24e; // Same as 6804?; */
        case 0xC0F07150: return 0x151; // change with vertical res change - 6804->664 by default value for
    }
    
    return reg_override_bits(reg, old_val);
}

//6d_1824x818 50fps
static inline uint32_t reg_override_HD50_6D(uint32_t reg, uint32_t old_val)
{
    
    EngDrvOutLV(0xc0f38024, 0x34f01dd); // fixes Canon liveview
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
        case 0xC0F06804: return 0x35101ea; // 1832 x 869 6d; */
            
        case 0xC0F06824: return 0x201;
        case 0xC0F06828: return 0x201;
        case 0xC0F0682C: return 0x201;
        case 0xC0F06830: return 0x201;
            
        case 0xC0F06010: return 0x203;
        case 0xC0F06008: return 0x2030203;
        case 0xC0F0600C: return 0x2030203;
            
        case 0xC0F06014: return 0x3df;
            
        case 0xC0F0713c: return 0x34f;
        case 0xC0F07150: return 0x34f;
    }
    
    return reg_override_bits(reg, old_val);
}

//6D 1216x2052 1x3 25fps
static inline uint32_t reg_override_1x3_6D(uint32_t reg, uint32_t old_val)
{
    //    EngDrvOutLV(0xc0f38024, 0x8210509); //try fixing liveview, can't get it right in this mode
    
    switch (reg)
    {
        case 0xC0F06804: return 0x8230150; // 1640 x 2052 25 fps
            
            //        case 0xC0F08184: return 0x821;
            //        case 0xC0F08188: return 0xc13;   //a13
            //        case 0xC0F08194: return 0xc13;   //a13
            //        case 0xC0F08198: return 0xc13;   //a13
            
        case 0xC0F06824: return 0x1ce;
        case 0xC0F06828: return 0x1ce;
        case 0xC0F0682C: return 0x1ce;
        case 0xC0F06830: return 0x1ce;
            
        case 0xC0F06010: return 0x1d0;
        case 0xC0F06008: return 0x1d001d0;
        case 0xC0F0600C: return 0x1d001d0;
            
        case 0xC0F06014: return 0x89a; //25 fps for 3x1
            
        case 0xC0F0713c: return 0x823;
        case 0xC0F07150: return 0x823;
    }
    
    return reg_override_bits(reg, old_val);
}

//6d 1640 x 2144 24 fps
static inline uint32_t reg_override_1x3_24FPS_6D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
        case 0xC0F06804: return 0x87f01ba; // 1640 x 2144 24 fps
            
        case 0xC0F06824: return 0x1ce;
        case 0xC0F06828: return 0x1ce;
        case 0xC0F0682C: return 0x1ce;
        case 0xC0F06830: return 0x1ce;
            
        case 0xC0F06010: return 0x1d0;
        case 0xC0F06008: return 0x1d001d0;
        case 0xC0F0600C: return 0x1d001d0;
            
        case 0xC0F06014: return 0x8F5; //24 fps for 3x1
            
        case 0xC0F0713c: return 0x87f;
        case 0xC0F07150: return 0x82f;
    }
    
    return reg_override_bits(reg, old_val);
}

//6D - 2688 x 1296 at 25fps
static inline uint32_t reg_override_2K_6D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
            /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804: return 0x52f02c0; // 2688x1296_25fps x5 Mode;
            
        case 0xC0F06824: return 0x2d7;
        case 0xC0F06828: return 0x2d7;
        case 0xC0F0682C: return 0x2d7;
        case 0xC0F06830: return 0x2d7;
            
        case 0xC0F06010: return 0x2d9;
        case 0xC0F06008: return 0x2d902d9;
        case 0xC0F0600C: return 0x2d902d9;
            
            /*case 0xC0F06014: return 0x5b4; for 24fps */
        case 0xC0F06014: return 0x57a; /*  for 25 fps  */
            
        case 0xC0F0713c: return 0x530;
        case 0xC0F07150: return 0x51f;
            
    }
    
    return reg_override_bits(reg, old_val);
}

//6D - 2880 x 1200 at 25fps
static inline uint32_t reg_override_ULTRA_HD_6D(uint32_t reg, uint32_t old_val)
{
    
    
    EngDrvOutLV(0xc0f38024, 0x4dc02e3); //fixes liveview - not correct framing, but working liveview
    
    switch (reg)
    
    {
            
        case 0xC0F06804: return 0x4ce02f0; // UHD for 6d   was 3dd02c0
            
        case 0xC0F06824: return 0x303;
        case 0xC0F06828: return 0x303;
        case 0xC0F0682C: return 0x303;
        case 0xC0F06830: return 0x303;
            
        case 0xC0F06010: return 0x305;
        case 0xC0F06008: return 0x3050305;
        case 0xC0F0600C: return 0x3050305;
            
            /* case 0xC0F06014: return 0x561; for 24 fps */
        case 0xC0F06014: return 0x52a; /* for 25 fps */
            
            
        case 0xC0F0713c: return 0x4ce;
        case 0xC0F07150: return 0x4ce;
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_4K_6D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column)*/
            /* 6804 = 5bf04b0 = 4672 x 1440 raw resolution (end line/column) */
            /* 6804 = 5bf03e0 = 3840 x 1440 raw resolution (end line/column) */
            /* 6804 = 5bf04e0 = 4864 x 1440 raw resolution (end line/column) */
            /* 6804 = 5bf0502 = 5000 x 1440 raw resolution (end line/column) */
            /* 6814 -> 5f0 -> 16.56fps(end line/column) Corrupted frames*/
            /* 6814 -> 610 -> 16.224fps(end line/column) NOcorrupted frames*/
            /* 6008 -> 3f7 -> 16.56fps(end line/column)  2a1 for 25fps 16:9 2.5K ?*/
            /*MAX about 6014=610 6008=3f7 6804 = 5bf03e0 at 16.2fps*/
            
            //case 0xC0F06804: return 0x88f03e0; // UHD for 6d
            
            //case 0xC0F06824: return 0x3f5;
            //case 0xC0F06828: return 0x3f5;
            //case 0xC0F0682C: return 0x3f5;
            //case 0xC0F06830: return 0x3f5;
            
            //case 0xC0F06010: return 0x3f7;
            //case 0xC0F06008: return 0x3f703f7;
            //case 0xC0F0600C: return 0x3f703f7;
            
            //case 0xC0F06014: return 0x9d7;
            
            //case 0xC0F0713c: return 0x88f;
            /*case 0xC0F07150: return 0xe32;*/
            //Werkt niet, wordt overruled door raw.c   case 0xC0F37014: return 0x12;
            
            
            
            
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_5K_6D(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
            /* raw resolution (end line/column) */
        case 0xC0F06804: return 0x2800520; // 5472x3604 5x zoom;
            
        case 0xC0F06824: return 0x535;
        case 0xC0F06828: return 0x535;
        case 0xC0F0682C: return 0x535;
        case 0xC0F06830: return 0x535;
            
        case 0xC0F06010: return 0x537;
        case 0xC0F06008: return 0x5370537;
        case 0xC0F0600C: return 0x5370537;
            
        case 0xC0F06014: return 0x2fe; //8fb;
            
        case 0xC0F0713c: return 0x280;
        case 0xC0F07150: return 0x27f;
    }
    
    return reg_override_bits(reg, old_val);
}

//6D 5464x3072 2.5fps 
static inline uint32_t reg_override_FullRes_6D(uint32_t reg, uint32_t old_val)
{
    //    EngDrvOutLV(0xc0f38024, 0x4dc0569); //fixes liveview - not correct framing, but working liveview
    
    switch (reg)
    {
            /* raw resolution (end line/column) */
        case 0xC0F06804: return 0xc1e0576; // 5472x3078 5x zoom;  c24
            
        case 0xC0F06824: return 0x58d;
        case 0xC0F06828: return 0x58d;
        case 0xC0F0682C: return 0x58d;
        case 0xC0F06830: return 0x58d;
            
        case 0xC0F06010: return 0x58f;
        case 0xC0F06008: return 0x58f058f;
        case 0xC0F0600C: return 0x58f058f;
            
        case 0xC0F06014: return 0x1c16; // for 2fps use 0x231c for 5.6 fps 0xc7f;
            
        case 0xC0F0713c: return 0xc1d;
        case 0xC0F07150: return 0xc01;
    }
    
    return reg_override_bits(reg, old_val);
}
//6D - 2480x1396 at 25fps
static inline uint32_t reg_override_HD2_6D(uint32_t reg, uint32_t old_val)
{
    EngDrvOutLV(0xc0f38024, 0x591027f); //fixes liveview - not correct framing, but working liveview
    
    switch (reg)
    {
        case 0xC0F06804: return 0x593028c; // 2480x1396_25fps x5 Mode;
            
        case 0xC0F06824: return 0x2a3;
        case 0xC0F06828: return 0x2a3;
        case 0xC0F0682C: return 0x2a3;
        case 0xC0F06830: return 0x2a3;
            
        case 0xC0F06010: return 0x2a5;
        case 0xC0F06008: return 0x2a502a5;
        case 0xC0F0600C: return 0x2a502a5;
            
        case 0xC0F06014: return 0x5e5; /*  for 25 fps  */
            
        case 0xC0F0713c: return 0x594;
        case 0xC0F07150: return 0x55f;
            
    }
    
    return reg_override_bits(reg, old_val);
}

static inline uint32_t reg_override_zoom_fps(uint32_t reg, uint32_t old_val)
{
    /* attempt to reconfigure the x5 zoom at the FPS selected in Canon menu */
    int timerA =
    (video_mode_fps == 24) ? 512 :
    (video_mode_fps == 25) ? 512 :
    (video_mode_fps == 30) ? 520 :
    (video_mode_fps == 50) ? 512 :  /* cannot get 50, use 25 */
    (video_mode_fps == 60) ? 520 :  /* cannot get 60, use 30 */
    -1 ;
    int timerB =
    (video_mode_fps == 24) ? 1955 :
    (video_mode_fps == 25) ? 1875 :
    (video_mode_fps == 30) ? 1540 :
    (video_mode_fps == 50) ? 1875 :
    (video_mode_fps == 60) ? 1540 :
    -1 ;
    
    return reg_override_fps_nocheck(reg, timerA, timerB, old_val);
}

static int engio_vidmode_ok = 0;

static void * get_engio_reg_override_func()
{
    uint32_t (*reg_override_func)(uint32_t, uint32_t) =
    (crop_preset == CROP_PRESET_3X)         ? reg_override_mv1080_mv720p     : /* fixme: corrupted image */
    (crop_preset == CROP_PRESET_3X_TALL)    ? reg_override_3X_tall    :
    (crop_preset == CROP_PRESET_3x3_1X_50p)     ? reg_override_3x3_50p   :
    (crop_preset == CROP_PRESET_3x3_1X_60p)     ? reg_override_3x3_60p   :
    (crop_preset == CROP_PRESET_3x3_1X_45p) ? reg_override_3x3_45p    :
    (crop_preset == CROP_PRESET_3x3_1X_48p) ? reg_override_3x3_48p    :
    (crop_preset == CROP_PRESET_3K)         ? reg_override_3K         :
    (crop_preset == CROP_PRESET_4K_HFPS)    ? reg_override_4K_hfps    :
    (crop_preset == CROP_PRESET_UHD)        ? reg_override_UHD        :
    (crop_preset == CROP_PRESET_40_FPS)     ? reg_override_40_fps     :
    (crop_preset == CROP_PRESET_FULLRES_LV) ? reg_override_fullres_lv :
    (crop_preset == CROP_PRESET_CENTER_Z)   ? reg_override_zoom_fps   :
    (crop_preset == CROP_PRESET_1x3)        ? reg_override_1x3 :
    (crop_preset == CROP_PRESET_1x3_17fps)  ? reg_override_1x3_17fps :
    (crop_preset == CROP_PRESET_mv1080_mv720p)    ? reg_override_mv1080_mv720p  :
    (crop_preset == CROP_PRESET_mv1080p_mv720p_100D)    ? reg_override_3xcropmode_100d  :
    (crop_preset == CROP_PRESET_3xcropmode_100D)    ? reg_override_3xcropmode_100d  :
    (crop_preset == CROP_PRESET_2K_100D)    ? reg_override_2K_100d         :
    (crop_preset == CROP_PRESET_3K_100D)    ? reg_override_3K_100d         :
    (crop_preset == CROP_PRESET_4K_100D)    ? reg_override_4K_100d         :
    (crop_preset == CROP_PRESET_4K_3x1_100D) 	     ? reg_override_4K_3x1_100D        :
    (crop_preset == CROP_PRESET_5K_3x1_100D) 	     ? reg_override_5K_3x1_100D        :
    (crop_preset == CROP_PRESET_1080K_100D)	     ? reg_override_1080p_100d      :
    (crop_preset == CROP_PRESET_anamorphic_rewired_100D) ? reg_override_anamorphic_rewired_100d        :
    (crop_preset == CROP_PRESET_CENTER_Z_EOSM) ? reg_override_center_z_eosm        :
    (crop_preset == CROP_PRESET_2K_EOSM)         ? reg_override_2K_eosm         :
    (crop_preset == CROP_PRESET_3K_EOSM)         ? reg_override_3K_eosm         :
    (crop_preset == CROP_PRESET_4K_EOSM) 	     ? reg_override_4K_eosm         :
    (crop_preset == CROP_PRESET_4K_3x1_EOSM) 	     ? reg_override_4K_3x1_EOSM        :
    (crop_preset == CROP_PRESET_5K_3x1_EOSM) 	     ? reg_override_5K_3x1_EOSM        :
    (crop_preset == CROP_PRESET_4K_5x1_EOSM) 	     ? reg_override_4K_5x1_EOSM        :
    (crop_preset == CROP_PRESET_3x3_mv1080_EOSM) ? reg_override_3x3_mv1080_eosm        :
    (crop_preset == CROP_PRESET_mcm_mv1080_EOSM) ? reg_override_mcm_mv1080_eosm        :
    (crop_preset == CROP_PRESET_3x3_mv1080_48fps_EOSM) ? reg_override_3x3_48fps_eosm        :
    (crop_preset == CROP_PRESET_3x3_mv1080_48fps_700D) ? reg_override_3x3_48fps_eosm        : //Same as for eosm?
    (crop_preset == CROP_PRESET_3x3_mv1080_48fps_650D) ? reg_override_3x3_48fps_eosm        : //Same as for eosm?
    (crop_preset == CROP_PRESET_3x1_mv720_50fps_EOSM) ? reg_override_3x1_mv720_50fps_eosm        :
    (crop_preset == CROP_PRESET_anamorphic_rewired_EOSM) ? reg_override_anamorphic_rewired_eosm        :
    (crop_preset == CROP_PRESET_anamorphic_EOSM) ? reg_override_anamorphic_eosm        :
    (crop_preset == CROP_PRESET_3x3_1X_EOSM)    ? reg_override_mv1080_mv720p  :
    (crop_preset == CROP_PRESET_3x3_1X_100D)    ? reg_override_mv1080_mv720p  :
    (crop_preset == CROP_PRESET_x10_EOSM)    ? reg_override_x10_eosm  :
    (crop_preset == CROP_PRESET_OFF_eosm)    ? reg_override_crop_preset_off_eosm  :
    (crop_preset == CROP_PRESET_2520_1418_700D)       ? reg_override_2520_700d            :
    (crop_preset == CROP_PRESET_2520_1418_650D)       ? reg_override_2520_700d            :
    (crop_preset == CROP_PRESET_3K_700D)         ? reg_override_3K_700d         :
    (crop_preset == CROP_PRESET_3K_650D)         ? reg_override_3K_700d         :
    (crop_preset == CROP_PRESET_3540_700D)       ? reg_override_3540            :
    (crop_preset == CROP_PRESET_3540_650D)       ? reg_override_3540            :
    (crop_preset == CROP_PRESET_4K_700D)         ? reg_override_4K_700d         :
    (crop_preset == CROP_PRESET_4K_650D)         ? reg_override_4K_700d         :
    (crop_preset == CROP_PRESET_UHD_700D)        ? reg_override_UHD_700d        :
    (crop_preset == CROP_PRESET_UHD_650D)        ? reg_override_UHD_700d        :
    (crop_preset == CROP_PRESET_FULLRES_LV_700D) ? reg_override_fullres_lv_700d :
    (crop_preset == CROP_PRESET_FULLRES_LV_650D) ? reg_override_fullres_lv_700d :
    (crop_preset == CROP_PRESET_3x3_mv1080_700D) ? reg_override_3x3_mv1080_700d     :
    (crop_preset == CROP_PRESET_3x3_mv1080_650D) ? reg_override_3x3_mv1080_700d     :
    (crop_preset == CROP_PRESET_anamorphic_700D) ? reg_override_anamorphic_700d        :
    (crop_preset == CROP_PRESET_anamorphic_650D) ? reg_override_anamorphic_700d        :
    (crop_preset == CROP_PRESET_CENTER_Z_700D) ? reg_override_center_z_700d        :
    (crop_preset == CROP_PRESET_CENTER_Z_650D) ? reg_override_center_z_700d        :
    (crop_preset == CROP_PRESET_HD100_6D)      ? reg_override_HD100_6D      :
    (crop_preset == CROP_PRESET_HD75_6D)       ? reg_override_HD75_6D       :
    (crop_preset == CROP_PRESET_HD50_6D)       ? reg_override_HD50_6D       :
    (crop_preset == CROP_PRESET_1x3_6D)        ? reg_override_1x3_6D        :
    (crop_preset == CROP_PRESET_1x3_24FPS_6D)  ? reg_override_1x3_24FPS_6D  :
    (crop_preset == CROP_PRESET_2K_6D)         ? reg_override_2K_6D         :
    (crop_preset == CROP_PRESET_ULTRA_HD_6D)   ? reg_override_ULTRA_HD_6D   :
    (crop_preset == CROP_PRESET_4K_6D)         ? reg_override_4K_6D         :
    (crop_preset == CROP_PRESET_5K_6D)         ? reg_override_5K_6D         :
    (crop_preset == CROP_PRESET_FullRes_6D)    ? reg_override_FullRes_6D    :
    (crop_preset == CROP_PRESET_HD2_6D)        ? reg_override_HD2_6D        :
    0                       ;
    return reg_override_func;
}

static void FAST engio_write_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    uint32_t (*reg_override_func)(uint32_t, uint32_t) =
    get_engio_reg_override_func();
    
    if (!reg_override_func)
    {
        return;
    }
    
    /* cmos_vidmode_ok doesn't help;
     * we can identify the current video mode from 0xC0F06804 */
    for (uint32_t * buf = (uint32_t *) regs[0]; *buf != 0xFFFFFFFF; buf += 2)
    {
        uint32_t reg = *buf;
        uint32_t old = *(buf+1);
        if (reg == 0xC0F06804)
        {
            if (is_5D3)
            {
                engio_vidmode_ok = (crop_preset == CROP_PRESET_CENTER_Z)
                ? (old == 0x56601EB)                        /* x5 zoom */
                : (old == 0x528011B || old == 0x2B6011B);   /* 1080p or 720p */
            }
            if (is_100D)
            {
                engio_vidmode_ok =
                (old == 0x45802A1) /* x5 zoom */ || (old == 0x42801ed) /* x3 digital zoom */ || (old == 0x4A701D7 || old == 0x2D801D7);   /* 1080p or 720p */
            }
            if (is_EOSM)
            {
                engio_vidmode_ok =
                (old == 0x4540298) /* x5 zoom */ || (old == 0x42401e4) /* x3 digital zoom */ || (old == 0x4a601d4) /* 1080p */ || (old == 0x2d701d4 /* 720p */);
            }
            if (is_700D)
            {
                engio_vidmode_ok =
                (old == 0x4540298) ||/* x5 zoom */ (old == 0x4a601d4 || old == 0x2d701d4);   /* 1080p or 720p */
            }
            if (is_650D)
            {
                engio_vidmode_ok =
                (old == 0x4540298) /* x5 zoom */ || (old == 0x42401e4) /* x3 digital zoom */ || (old == 0x4a601d4) /* 1080p */ || (old == 0x2d701d4 /* 720p */);
            }
            if (is_6D)
            {
                engio_vidmode_ok =
                (old == 0x3dd02c0) ||/* x5 zoom */ (old == 0x4e501ec || old == 0x29801ec);   /* 1080p or 720p */
            }
            
        }
    }
    
    if (!is_supported_mode() || !engio_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }
    
    for (uint32_t * buf = (uint32_t *) regs[0]; *buf != 0xFFFFFFFF; buf += 2)
    {
        uint32_t reg = *buf;
        uint32_t old = *(buf+1);
        
        int new = reg_override_func(reg, old);
        if (new)
        {
            dbg_printf("[%x] %x: %x -> %x\n", regs[0], reg, old, new);
            *(buf+1) = new;
        }
    }
}

static int patch_active = 0;

static void update_patch()
{
    
    if (CROP_PRESET_MENU)
    {
        /* update preset */
        crop_preset = CROP_PRESET_MENU;
        
        /* install our hooks, if we haven't already do so */
        if (!patch_active)
        {
            patch_hook_function(CMOS_WRITE, MEM_CMOS_WRITE, &cmos_hook, "crop_rec: CMOS[1,2,6] parameters hook");
            patch_hook_function(ADTG_WRITE, MEM_ADTG_WRITE, &adtg_hook, "crop_rec: ADTG[8000,8806] parameters hook");
            if (ENGIO_WRITE)
            {
                patch_hook_function(ENGIO_WRITE, MEM_ENGIO_WRITE, engio_write_hook, "crop_rec: video timers hook");
            }
            
            patch_active = 1;
        }
    }
    else
    {
        /* undo active patches, if any */
        if (patch_active)
        {
            unpatch_memory(CMOS_WRITE);
            unpatch_memory(ADTG_WRITE);
            if (ENGIO_WRITE)
            {
                unpatch_memory(ENGIO_WRITE);
            }
            patch_active = 0;
            crop_preset = 0;
        }
    }
}

/* enable patch when switching LiveView (not in the middle of LiveView) */
/* otherwise you will end up with a halfway configured video mode that looks weird */
PROP_HANDLER(PROP_LV_ACTION)
{
    update_patch();
}

/* also try when switching zoom modes */
PROP_HANDLER(PROP_LV_DISPSIZE)
{
    update_patch();
}

static MENU_UPDATE_FUNC(crop_update)
{
    if ((CROP_PRESET_MENU && lv) && !is_100D && !is_EOSM && !is_700D && !is_650D)
    {
        if (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z ||
            crop_preset == CROP_PRESET_CENTER_Z_700D ||
            crop_preset == CROP_PRESET_CENTER_Z_650D )
        {
            if (lv_dispsize == 1)
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "To use this mode, exit ML menu & press the zoom button (set to x5/x10).");
            }
        }
        else /* non-zoom modes */
        {
            if (!is_supported_mode())
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in 1080p and 720p video modes.");
            }
            else if (lv_dispsize != 1)
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "To use this mode, exit ML menu and press the zoom button (set to x1).");
            }
            else if (!is_720p())
            {
                if (CROP_PRESET_MENU == CROP_PRESET_3x3_1X ||
                    CROP_PRESET_MENU == CROP_PRESET_3x3_1X_50p ||
                    CROP_PRESET_MENU == CROP_PRESET_3x3_1X_60p ||
                    CROP_PRESET_MENU == CROP_PRESET_3x3_1X_45p ||
                    CROP_PRESET_MENU == CROP_PRESET_3x3_1X_48p)
                {
                    /* these presets only have effect in 720p mode */
                    MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in the 720p 50/60 fps modes from Canon menu.");
                    return;
                }
            }
        }
    }
    
    if ((CROP_PRESET_MENU && lv) && (is_EOSM))
    {
        
        if ((lv_dispsize == 1) &&
            ((CROP_PRESET_MENU == CROP_PRESET_4K_3x1_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_2K_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_3K_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_4K_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_EOSM)))
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in x5 zoom");
            return;
        }
        
        if ((lv_dispsize > 1) &&
            ((CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_3x1_mv720_50fps_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM)
             || (CROP_PRESET_MENU == CROP_PRESET_anamorphic_EOSM)))
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in x1 movie mode");
            return;
        }
        
    }
    
    if ((CROP_PRESET_MENU && lv) && (is_100D))
    {
        
        if ((lv_dispsize == 1) &&
            ((CROP_PRESET_MENU == CROP_PRESET_4K_3x1_100D)
             || (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_100D)
             || (CROP_PRESET_MENU == CROP_PRESET_2K_100D)
             || (CROP_PRESET_MENU == CROP_PRESET_3K_100D)
             || (CROP_PRESET_MENU == CROP_PRESET_4K_100D)
             || (CROP_PRESET_MENU == CROP_PRESET_1080K_100D)))
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in x5 zoom");
            return;
        }
    }
    
    if (!is_720p() && ((CROP_PRESET_MENU == CROP_PRESET_3x3_1X_100D) || (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_700D) || (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_650D)))
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in mv720p movie mode");
        return;
    }
    
    if ((lv_dispsize > 1) &&
        ((CROP_PRESET_MENU == CROP_PRESET_3x3_1X_100D)
         || (CROP_PRESET_MENU == CROP_PRESET_mv1080p_mv720p_100D)
         || (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D)
         || (CROP_PRESET_MENU == CROP_PRESET_3xcropmode_100D)))
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This preset only works in x1 movie mode");
        return;
    }
}

static MENU_UPDATE_FUNC(target_yres_update)
{
    MENU_SET_RINFO("from %d", max_resolutions[crop_preset][get_video_mode_index()]);
}

static struct menu_entry crop_rec_menu[] =
{
    {
        .name       = "Crop mode",
        .priv       = &crop_preset_index,
        .update     = crop_update,
        .depends_on = DEP_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name   = "bitdepth",
                .priv   = &bitdepth,
                .max    = 4,
                .choices = CHOICES("OFF", "8 bit", "9 bit", "10 bit", "12 bit"),
                .help   = "Alter bitdepth\n"
            },
            {
                .name   = "ratios",
                .priv   = &ratios,
                .max    = 3,
                .choices = CHOICES("OFF", "2.39:1", "2.35:1", "16:9"),
                .help   = "Change aspect ratio\n"
            },
            {
                .name   = "x3crop",
                .priv   = &x3crop,
                .max    = 1,
                .choices = CHOICES("OFF", "ON"),
                .help   = "Turns mv1080p and mv1080_46fps modes into x3 crop modes)",
            },
            /*
            {
                .name   = "x3crop toggle",
                .priv   = &x3toggle,
                .max    = 2,
                .choices = CHOICES("OFF", "press down", "SET"),
                .help   = "In and out of x3crop(all mv1080p modes)",
                .help2  = "Select a short press key(EOSM)\n"
            },
             */
            {
                .name   = "focus aid",
                .priv   = &zoomaid,
                .max    = 2,
                .choices = CHOICES("OFF", "default mode", "dark mode"),
                .help   = "x10 zoom when pressing halfshutter(all presets, manual focus)",
                .help2   = "Will brighten liveview(slower fps)\n"
            },
            {
                .name   = "set 25fps",
                .priv   = &set_25fps,
                .max    = 1,
                .choices = CHOICES("OFF", "ON"),
                .help   = "OFF = default 24fps, ON = 25fps",
                .help2  = "Please apply 25fps also in canon menu for correct shutter handling\n"
            },
           /*
            {
                .name   = "iso climb",
                .priv   = &isoclimb,
                .max    = 1,
                .choices = CHOICES("OFF", "ON", "ON", "ON", "ON", "ON", "ON"),
                .help   = "Fast access to iso (NOT working with autoiso)",
                .help2  = "push up/down(eosm), SET(100d) button 100-3200 iso\n"
            },
            */
            {
                .name   = "max iso",
                .priv   = &isoauto,
                .max    = 3,
                .choices = CHOICES("OFF", "400", "800", "1600"),
                .help   = "Restrict autoiso to max 400/800/1600",
                .help2  = "Select max iso. Turn on autoiso\n"
            },
            {
                .name   = "4k timelapse",
                .priv   = &timelapse,
                .max    = 9,
                .choices = CHOICES("OFF", "0.4fps" ,"1fps", "2fps", "3fps", "4fps", "5fps","5K 0.4fps", "5K 1fps", "5K 2fps"),
                .help   = "Fps intervals(only 4k preset(100D/EOSM)\n",
                .help2  = "Enable REC trigger Half-shutter: start/pause for best performance",
            },
            {
                .name   = "Slow shutter",
                .priv   = &slowshutter,
                .max    = 1,
                .choices = CHOICES("OFF", "ON"),
                .help   = "Allows for slow shutter speeds with 4k timelapse(Only 100D/EOSM).\n"
            },
            {
                .name   = "Startoff presets",
                .priv   = &presets,
                .max    = 6,
                .choices = CHOICES("None selected", "mv1080p MCM rewire 14bit", "mv1080p MCM rewire 14bit x3crop", "5K anamorphic 10bit", "2.5K 10bit", "mv1080p 10bit 45/48/50fps", "mv1080p 10bit 45/48/50fps x3crop"),
                .help   = "Select startoff preset(EOSM only)",
                .help2  = "Select ratio and fps in Crop mode submenu",
            },
            {
                .name   = "hdr iso A",
                .priv   = &HDR_iso_a,
                .max    = 6,
                .choices = CHOICES("OFF", "iso100", "iso200", "iso400", "iso800", "iso1600", "iso3200"),
                .help   =  "HDR workaround eosm",
                .advanced = 1,
            },
            {
                .name   = "hdr iso B",
                .priv   = &HDR_iso_b,
                .max    = 6,
                .choices = CHOICES("OFF", "iso100", "iso200", "iso400", "iso800", "iso1600", "iso3200"),
                .help   =  "HDR workaround eosm",
                .advanced = 1,
            },
            {
                .name   = "reg_8",
                .priv   = &reg_8,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "reg 8",
                .advanced = 1,
            },
            {
                .name   = "reg_9",
                .priv   = &reg_9,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "reg 9",
                .advanced = 1,
            },
            {
                .name   = "reg_a",
                .priv   = &reg_a,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "reg a",
                .advanced = 1,
            },
            {
                .name   = "reg_b",
                .priv   = &reg_b,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "reg b",
                .advanced = 1,
            },
            {
                .name   = "reg_fe",
                .priv   = &reg_fe,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "reg fe",
                .advanced = 1,
            },
            {
                .name   = "reg_713c",
                .priv   = &reg_713c,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "Corruption? Combine with reg_7150",
                .advanced = 1,
            },
            {
                .name   = "reg_7150",
                .priv   = &reg_7150,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "Corruption issues? Combine with reg_713c",
                .advanced = 1,
            },
            {
                .name   = "reg_6014",
                .priv   = &reg_6014,
                .min    = -100000,
                .max    = 100000,
                .unit   = UNIT_DEC,
                .help  = "Alter frame rate. Combine with reg_6008",
                .advanced = 1,
            },
            {
                .name   = "reg_6008",
                .priv   = &reg_6008,
                .min    = -10000,
                .max    = 10000,
                .unit   = UNIT_DEC,
                .help  = "Alter frame rate. Combine with reg_6014",
                .advanced = 1,
            },
            {
                .name   = "reg_800c",
                .priv   = &reg_800c,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "line skipping",
                .advanced = 1,
            },
            {
                .name   = "reg_8000",
                .priv   = &reg_8000,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "x3zoom",
                .advanced = 1,
            },
            {
                .name   = "reg_8183",
                .priv   = &reg_8183,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "Aliasing, moiré mcm rewired mode",
                .advanced = 1,
            },
            {
                .name   = "reg_8184",
                .priv   = &reg_8184,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "Aliasing, moiré mcm rewired mode",
                .advanced = 1,
            },
            {
                .name   = "reg_6800_height",
                .priv   = &reg_6800_height,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "height offset",
                .advanced = 1,
            },
            {
                .name   = "reg_6800_width",
                .priv   = &reg_6800_width,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "width offset",
                .advanced = 1,
            },
            {
                .name   = "reg_6804_height",
                .priv   = &reg_6804_height,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "Alter height.",
                .advanced = 1,
            },
            {
                .name   = "reg_6804_width",
                .priv   = &reg_6804_width,
                .min    = -2000,
                .max    = 2000,
                .unit   = UNIT_DEC,
                .help  = "Alter width. Scrambles preview",
                .advanced = 1,
            },
            {
                .name   = "reg_83d4",
                .priv   = &reg_83d4,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "Preview engdrvout",
                .advanced = 1,
            },
            {
                .name   = "reg_83dc",
                .priv   = &reg_83dc,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "Preview engdrvout",
                .advanced = 1,
            },
            {
                .name   = "CMOS[1]",
                .priv   = &cmos1,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Vertical offset",
                .advanced = 1,
            },
            {
                .name   = "CMOS[2]",
                .priv   = &cmos2,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Horizontal offset",
                .advanced = 1,
            },
            {
                .name   = "CMOS[3]",
                .priv   = &cmos3,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Analog iso on 6D",
                .advanced = 1,
            },
            {
                .name   = "CMOS[4]",
                .priv   = &cmos4,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Hot/cold pixel",
                .advanced = 1,
            },
            {
                .name   = "CMOS[5]",
                .priv   = &cmos5,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Fine vertical offset, black area maybe",
                .advanced = 1,
            },
            {
                .name   = "CMOS[6]",
                .priv   = &cmos6,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Iso 50 or timing related",
                .advanced = 1,
            },
            {
                .name   = "CMOS[7]",
                .priv   = &cmos7,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Image fading out; 6D, 700D: vertical offset",
                .advanced = 1,
            },
            {
                .name   = "CMOS[8]",
                .priv   = &cmos8,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "Unknown, used on 6D",
                .help2  = "Use for horizontal centering.",
                .advanced = 1,
            },
            {
                .name   = "CMOS[9]",
                .priv   = &cmos9,
                .max    = 0xFFF,
                .unit   = UNIT_HEX,
                .help   = "?",
                .advanced = 1,
            },
            {
                .name   = "reg_gain",
                .priv   = &reg_gain,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "Alter bit depth",
                .advanced = 1,
            },
            {
                .name   = "reg_timing1",
                .priv   = &reg_timing1,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 8172, 8178, 8196",
                .advanced = 1,
            },
            {
                .name   = "reg_timing2",
                .priv   = &reg_timing2,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 8179, 8197",
                .advanced = 1,
            },
            {
                .name   = "reg_timing3",
                .priv   = &reg_timing3,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 8173",
                .advanced = 1,
            },
            {
                .name   = "reg_timing4",
                .priv   = &reg_timing4,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 82f8, 82f9",
                .advanced = 1,
            },
            {
                .name   = "reg_timing5",
                .priv   = &reg_timing5,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 6014",
                .advanced = 1,
            },
            {
                .name   = "reg_timing6",
                .priv   = &reg_timing6,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 82b6",
                .advanced = 1,
            },
            {
                .name   = "reg_6824",
                .priv   = &reg_6824,
                .min    = -500,
                .max    = 500,
                .unit   = UNIT_DEC,
                .help  = "PowSaveTim reg 6824, 6828, 682c, 6830",
                .advanced = 1,
            },
            {
                .name   = "reg_skip_left",
                .priv   = &reg_skip_left,
                .min    = -1000,
                .max    = 1000,
                .unit   = UNIT_DEC,
                .help  = "skip left",
                .advanced = 1,
            },
            {
                .name   = "reg_skip_right",
                .priv   = &reg_skip_right,
                .min    = -1000,
                .max    = 1000,
                .unit   = UNIT_DEC,
                .help  = "skip right",
                .advanced = 1,
            },
            {
                .name   = "reg_skip_top",
                .priv   = &reg_skip_top,
                .min    = -1000,
                .max    = 1000,
                .unit   = UNIT_DEC,
                .help  = "skip top",
                .advanced = 1,
            },
            {
                .name   = "reg_skip_bottom",
                .priv   = &reg_skip_bottom,
                .min    = -1000,
                .max    = 1000,
                .unit   = UNIT_DEC,
                .help  = "skip bottom",
                .advanced = 1,
            },
            {
                .name       = "Shutter range",
                .priv       = &shutter_range,
                .max        = 1,
                .choices    = CHOICES("Original", "Full range"),
                .help       = "Choose the available shutter speed range:",
                .help2      = "Original: default range used by Canon in selected video mode.\n"
                "Full range: from 1/FPS to minimum exposure time allowed by hardware.",
                .advanced = 1,
            },
            {
                .name   = "reg_bl",
                .priv   = &reg_bl,
                .min    = -1000,
                .max    = 1000,
                .unit   = UNIT_DEC,
                .help  = "black level for reduced bitdepths(not affecting recordings)",
                .advanced = 1,
            },
            /* not used atm	{
             .name   = "CMOS[1] lo",
             .priv   = &cmos1_lo,
             .max    = 63,
             .unit   = UNIT_DEC,
             .help   = "Start scanline (very rough). Use for vertical positioning.",
             .advanced = 1,
             },
             {
             .name   = "CMOS[1] hi",
             .priv   = &cmos1_hi,
             .max    = 63,
             .unit   = UNIT_DEC,
             .help   = "End scanline (very rough). Increase if white bar at bottom.",
             .help2  = "Decrease if you get strange colors as you move the camera.",
             .advanced = 1,
             },
             {
             .name   = "Target YRES",
             .priv   = &target_yres,
             .update = target_yres_update,
             .max    = 3870,
             .unit   = UNIT_DEC,
             .help   = "Desired vertical resolution (only for presets with higher resolution).",
             .help2  = "Decrease if you get corrupted frames (dial the desired resolution here).",
             .advanced = 1,
             },
             {
             .name   = "Delta ADTG 0",
             .priv   = &delta_adtg0,
             .min    = -500,
             .max    = 500,
             .unit   = UNIT_DEC,
             .help   = "ADTG 0x8178, 0x8196, 0x82F8",
             .help2  = "May help pushing the resolution a little. Start with small increments.",
             .advanced = 1,
             },
             {
             .name   = "Delta ADTG 1",
             .priv   = &delta_adtg1,
             .min    = -500,
             .max    = 500,
             .unit   = UNIT_DEC,
             .help   = "ADTG 0x8179, 0x8197, 0x82F9",
             .help2  = "May help pushing the resolution a little. Start with small increments.",
             .advanced = 1,
             },
             {
             .name   = "Delta HEAD3",
             .priv   = &delta_head3,
             .min    = -500,
             .max    = 500,
             .unit   = UNIT_DEC,
             .help2  = "May help pushing the resolution a little. Start with small increments.",
             .advanced = 1,
             },
             {
             .name   = "Delta HEAD4",
             .priv   = &delta_head4,
             .min    = -500,
             .max    = 500,
             .unit   = UNIT_DEC,
             .help2  = "May help pushing the resolution a little. Start with small increments.",
             .advanced = 1,
             }, */
            MENU_ADVANCED_TOGGLE,
            MENU_EOL,
        },
    },
};

static unsigned int crop_rec_keypress_cbr(unsigned int key)
{
    
    /* x3crop toggle by using short press on thrash can button instead of halfshutter */
    if (is_EOSM && lv && !gui_menu_shown() && !RECORDING && is_movie_mode() &&
        ((key == MODULE_KEY_PRESS_DOWN && x3toggle == 0x1) || (key == MODULE_KEY_PRESS_SET && x3toggle == 0x2)) &&
        (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_EOSM || CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM ||
         CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_EOSM))
    {
        if (x3crop == 0x1)
        {
            x3crop = 0x0;
        }
        else
        {
            x3crop = 0x1;
        }
        PauseLiveView();
        ResumeLiveView();
        
        return 0;
    }
    
    /* iso climbing feature */
    if ((!is_6D && isopatch && lv && !gui_menu_shown() && is_movie_mode()) &&
        (((is_EOSM && (key == MODULE_KEY_PRESS_DOWN || key == MODULE_KEY_PRESS_UP)) ||
          ((!is_EOSM && !is_5D3) && key == MODULE_KEY_PRESS_SET)) && isoclimb != 0x0 && HDR_iso_a == 0x0))
    {
        isopatch = 0;
        isopatchoff = 0;
        if (shamem_read(0xC0F0b12c) == 0x11)
        {
            if (key == MODULE_KEY_PRESS_UP) isoclimb = 0x2;
            if (key == MODULE_KEY_PRESS_DOWN) isoclimb = 0x6;
        }
        else if (shamem_read(0xC0F0b12c) == 0x12)
        {
            if (key == MODULE_KEY_PRESS_UP) isoclimb = 0x3;
            if (key == MODULE_KEY_PRESS_DOWN) isoclimb = 0x1;
        }
        else if (shamem_read(0xC0F0b12c) == 0x13)
        {
            if (key == MODULE_KEY_PRESS_UP) isoclimb = 0x4;
            if (key == MODULE_KEY_PRESS_DOWN) isoclimb = 0x2;
        }
        else if (shamem_read(0xC0F0b12c) == 0x14)
        {
            if (key == MODULE_KEY_PRESS_UP) isoclimb = 0x5;
            if (key == MODULE_KEY_PRESS_DOWN) isoclimb = 0x3;
        }
        else if (shamem_read(0xC0F0b12c) == 0x15)
        {
            if (key == MODULE_KEY_PRESS_UP) isoclimb = 0x6;
            if (key == MODULE_KEY_PRESS_DOWN) isoclimb = 0x4;
        }
        else if (shamem_read(0xC0F0b12c) == 0x16)
        {
            if (key == MODULE_KEY_PRESS_UP) isoclimb = 0x1;
            if (key == MODULE_KEY_PRESS_DOWN) isoclimb = 0x5;
        }
        return 0;
    }
    
    return 1;
}


static int crop_rec_needs_lv_refresh()
{
    if (!lv)
    {
        return 0;
    }
    
    /* startoff presets(experimental) */
    if (is_EOSM && presets != 0x0 && !RECORDING)
    {
        
        if (presets == 0x1)
        {
            NotifyBox(2000, "mv1080p MCM rewire 14bit");
            crop_preset_index = 1;
            bitdepth = 0x0;
            zoomaid = 0x1;
            if (isoclimb == 0x0) isoclimb = 0x1;
            x3crop = 0x0;
            x3toggle = 0x2;
            PauseLiveView();
            ResumeLiveView();
            presets = 0x0;
            return 0;
        }
        
        if (presets == 0x2)
        {
            NotifyBox(2000, "mv1080p MCM rewire 14bit x3crop");
            crop_preset_index = 1;
            bitdepth = 0x0;
            zoomaid = 0x1;
            if (isoclimb == 0x0) isoclimb = 0x1;
            x3crop = 0x1;
            x3toggle = 0x2;
            PauseLiveView();
            ResumeLiveView();
            presets = 0x0;
            return 0;
        }
        
        if (presets == 0x3)
        {
            NotifyBox(2000, "5K anamorphic 10bit");
            crop_preset_index = 10;
            bitdepth = 0x3;
            zoomaid = 0x1;
            if (isoclimb == 0x0) isoclimb = 0x1;
            x3crop = 0x0;
            x3toggle = 0x2;
            PauseLiveView();
            ResumeLiveView();
            set_lv_zoom(1);
            presets = 0x0;
            return 0;
        }
        
        if (presets == 0x4)
        {
            NotifyBox(2000, "2.5K 10bit");
            crop_preset_index = 6;
            bitdepth = 0x3;
            zoomaid = 0x1;
            if (isoclimb == 0x0) isoclimb = 0x1;
            x3crop = 0x0;
            x3toggle = 0x2;
            PauseLiveView();
            ResumeLiveView();
            presets = 0x0;
            return 0;
        }
        
        if (presets == 0x5)
        {
            NotifyBox(2000, "mv1080p 10bit 45/48/50fps");
            crop_preset_index = 2;
            bitdepth = 0x3;
            zoomaid = 0x1;
            if (isoclimb == 0x0) isoclimb = 0x1;
            x3crop = 0x0;
            x3toggle = 0x2;
            PauseLiveView();
            ResumeLiveView();
            set_lv_zoom(1);
            presets = 0x0;
            return 0;
        }
        
        if (presets == 0x6)
        {
            NotifyBox(2000, "mv1080p 10bit 45/48/50fps x3crop");
            crop_preset_index = 2;
            bitdepth = 0x3;
            zoomaid = 0x1;
            if (isoclimb == 0x0) isoclimb = 0x1;
            x3crop = 0x1;
            x3toggle = 0x2;
            PauseLiveView();
            ResumeLiveView();
            set_lv_zoom(1);
            presets = 0x0;
            return 0;
        }
        
    }
    
    /* We don´t want this when in photo mode I assume */
    if (!is_movie_mode()) return 0;
    
    /* let´s automate liveview start off setting */
    if ((CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_EOSM) ||
        (CROP_PRESET_MENU == CROP_PRESET_2K_100D) ||
        (CROP_PRESET_MENU == CROP_PRESET_3K_100D) ||
        (CROP_PRESET_MENU == CROP_PRESET_4K_100D) ||
        (CROP_PRESET_MENU == CROP_PRESET_2K_EOSM) ||
        (CROP_PRESET_MENU == CROP_PRESET_3K_EOSM) ||
        (CROP_PRESET_MENU == CROP_PRESET_4K_EOSM) ||
        (CROP_PRESET_MENU == CROP_PRESET_4K_3x1_EOSM) ||
        (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_EOSM) ||
        (CROP_PRESET_MENU == CROP_PRESET_4K_3x1_100D) ||
        (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_100D) ||
        (CROP_PRESET_MENU == CROP_PRESET_1080K_100D) ||
        (CROP_PRESET_MENU == CROP_PRESET_2520_1418_700D) ||
        (CROP_PRESET_MENU == CROP_PRESET_3K_700D) ||
        (CROP_PRESET_MENU == CROP_PRESET_3540_700D) ||
        (CROP_PRESET_MENU == CROP_PRESET_4K_700D) ||
        (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_700D) ||
        (CROP_PRESET_MENU == CROP_PRESET_2520_1418_650D) ||
        (CROP_PRESET_MENU == CROP_PRESET_3K_650D) ||
        (CROP_PRESET_MENU == CROP_PRESET_3540_650D) ||
        (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_650D) ||
        (CROP_PRESET_MENU == CROP_PRESET_4K_650D) ||
        (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z))
    {
        info_led_on();
        gui_uilock(UILOCK_EVERYTHING);
        set_lv_zoom(5);
        gui_uilock(UILOCK_NONE);
        info_led_off();
    }
    else
    {
        if (is_EOSM || is_100D || is_700D || is_650D)
        {
            info_led_on();
            gui_uilock(UILOCK_EVERYTHING);
            set_lv_zoom(1);
            gui_uilock(UILOCK_NONE);
            info_led_off();
        }
    }
    
    /* Update liveview in different ways depending on mcm rewired modes */
    if (is_EOSM && (shamem_read(0xc0f383d4) == 0x4f0010 &&
                    (CROP_PRESET_MENU != CROP_PRESET_mcm_mv1080_EOSM)) &&
        (CROP_PRESET_MENU != CROP_PRESET_anamorphic_rewired_EOSM))
    {
        /* mimics canon menu push and back. Needed to get mcm rewired regs updated */
        PauseLiveView();
        ResumeLiveView();
    }
    
    /* Update liveview in different ways depending on mcm rewired modes */
    if (is_EOSM && (shamem_read(0xc0f383d4) == 0x4f0010 && (shamem_read(0xc0f06804) == 0x4a601e4) &&
                    (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM)))
    {
        /* mimics canon menu push and back. Needed to get mcm rewired regs updated */
        PauseLiveView();
        ResumeLiveView();
    }
    
    /* Update liveview in different ways depending on mcm rewired modes */
    if (is_EOSM && (shamem_read(0xc0f383d4) == 0x4f0010 && (shamem_read(0xc0f06804) == 0x45601e4) &&
                    (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM)))
    {
        /* mimics canon menu push and back. Needed to get mcm rewired regs updated */
        PauseLiveView();
        ResumeLiveView();
    }
    
    if (is_EOSM && shamem_read(0xc0f383d4) == 0x4f0010 && (shamem_read(0xC0f0b13c) == 0xd) &&
        (CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM))
    {
        /* going from CROP_PRESET_anamorphic_rewired_EOSM to CROP_PRESET_mcm_mv1080_EOSM */
        PauseLiveView();
        ResumeLiveView();
    }
    
    /* Update liveview in different ways depending on mcm rewired modes */
    if (is_100D && shamem_read(0xc0f383d4) == 0x4f0010 &&
        (CROP_PRESET_MENU != CROP_PRESET_anamorphic_rewired_100D))
    {
        /* mimics canon menu push and back. Needed to get mcm rewired regs updated */
        PauseLiveView();
        ResumeLiveView();
    }
    
    if (CROP_PRESET_MENU)
    {
        if (is_supported_mode() || is_100D || is_EOSM)
        {
            if (!patch_active || CROP_PRESET_MENU != crop_preset || is_EOSM || is_100D || is_700D || is_650D || is_6D || is_5D3)
            {
                return 1;
            }
        }
    }
    else /* crop disabled */
    {
        if (patch_active || CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM || CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM || CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D)
        {
            return 1;
        }
    }
    
    return 0;
}

static void center_canon_preview()
{
    /* center the preview window on the raw buffer */
    /* overriding these registers once will do the trick...
     * ... until the focus box is moved by the user */
    int old = cli();
    
    uint32_t pos1 = shamem_read(0xc0f383d4);
    uint32_t pos2 = shamem_read(0xc0f383dc);
    
    if ((pos1 & 0x80008000) == 0x80008000 &&
        (pos2 & 0x80008000) == 0x80008000)
    {
        /* already centered */
        sei(old);
        return;
    }
    
    int x1 = pos1 & 0xFFFF;
    int x2 = pos2 & 0xFFFF;
    int y1 = pos1 >> 16;
    int y2 = pos2 >> 16;
    
    if (x2 - x1 != 299 && y2 - y1 != 792)
    {
        /* not x5/x10 (values hardcoded for 5D3) */
        sei(old);
        return;
    }
    
    int raw_xc = (146 + 3744) / 2 / 4;  /* hardcoded for 5D3 */
    int raw_yc = ( 60 + 1380) / 2;      /* values from old raw.c */
    
    if (1)
    {
        /* use the focus box position for moving the preview window around */
        /* don't do that while recording! */
        dbg_printf("[crop_rec] %d,%d ", raw_xc, raw_yc);
        raw_xc -= 146 / 2 / 4;  raw_yc -= 60 / 2;
        /* this won't change the position if the focus box is centered */
        get_afframe_pos(raw_xc * 2, raw_yc * 2, &raw_xc, &raw_yc);
        raw_xc += 146 / 2 / 4;  raw_yc += 60 / 2;
        raw_xc &= ~1;   /* just for consistency */
        raw_yc &= ~1;   /* this must be even, otherwise the image turns pink */
        raw_xc = COERCE(raw_xc, 176, 770);  /* trial and error; image pitch changes if we push to the right */
        raw_yc = COERCE(raw_yc, 444, 950);  /* trial and error; broken image at the edges, outside these limits */
        dbg_printf("-> %d,%d using focus box position\n", raw_xc, raw_yc);
    }
    int current_xc = (x1 + x2) / 2;
    int current_yc = (y1 + y2) / 2;
    int dx = raw_xc - current_xc;
    int dy = raw_yc - current_yc;
    
    if (dx || dy)
    {
        /* note: bits 0x80008000 appear to have no effect,
         * so we'll use them to flag the centered zoom mode,
         * e.g. for focus_box_get_raw_crop_offset */
        dbg_printf("[crop_rec] centering zoom preview: dx=%d, dy=%d\n", dx, dy);
        EngDrvOutLV(0xc0f383d4, PACK32(x1 + dx, y1 + dy) | 0x80008000);
        EngDrvOutLV(0xc0f383dc, PACK32(x2 + dx, y2 + dy) | 0x80008000);
    }
    
    sei(old);
}

/* faster version than the one from ML core */
static void set_zoom(int zoom)
{
    if (RECORDING) return;
    if (is_movie_mode() && video_mode_crop) return;
    zoom = COERCE(zoom, 1, 10);
    if (zoom > 1 && zoom < 10) zoom = 5;
    prop_request_change_wait(PROP_LV_DISPSIZE, &zoom, 4, 1000);
}


/* when closing ML menu, check whether we need to refresh the LiveView */
static unsigned int crop_rec_polling_cbr(unsigned int unused)
{
    
    //NotifyBox(2000, "lens_info.raw_iso_auto 0x%x", lens_info.raw_iso_auto);
    
    /* Needs refresh when turning off isoclimb or iso metadata will still be last selected iso climb setting */
    if (!isoclimb && !isopatchoff && (is_EOSM || is_100D))
    {
        isopatchoff = 1;
        if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM || CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM ||
            CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D)
        {
            movie_crop_hack_disable();
        }
        else
        {
            /* will try to enable mcm but will immediately be disabled. Side effect it will reset which is what we want. What to do with 5D3? */
            movie_crop_hack_enable();
        }
        return 0;
    }
    
    /* For when in photo mode and enabled x10 zoom mode */
    if (((zoomaid == 0x1 || zoomaid == 0x2) && !is_movie_mode()) || ((is_6D || is_5D3) && (!RECORDING && (zoomaid == 0x1 || zoomaid == 0x2))))
    {
        static int last_hs_photo = 0;
        static int photo = 0;
        
        if (!get_halfshutter_pressed()) last_hs_photo = get_ms_clock();
        if (get_ms_clock() - last_hs_photo > 500 && get_halfshutter_pressed())
        {
            set_lv_zoom(10);
            photo = 1;
            while (get_halfshutter_pressed())
            {
                msleep(10);
            }
        }
        
        if (!get_halfshutter_pressed() && photo)
        {
            set_lv_zoom(1);
            photo = 0;
            return 0;
        }
        
    }
    
    static int once1 = 1;
    if (once1 && (zoomaid == 0x1 || zoomaid == 0x2) && !is_movie_mode())
    {
        once1 = 0;
        NotifyBox(4000, "Crop mode x10 halfshutter focus aid active");
    }
    /* reset this notification once back in movie mode */
    if (is_movie_mode())
    {
        once1 = 1;
    }
    
    /* We don´t want this when in photo mode I assume */
    if (!is_movie_mode() || CROP_PRESET_MENU == CROP_PRESET_OFF) return 0;
    
    /* also check at startup */
    static int lv_dirty = 1;
    
    
    int menu_shown = gui_menu_shown();
    if (lv && menu_shown)
    {
        lv_dirty = 1;
    }
    
    if (!lv || menu_shown || RECORDING_RAW)
    {
        /* outside LV: no need to do anything */
        /* don't change while browsing the menu, but shortly after closing it */
        /* don't change while recording raw, but after recording stops
         * (H.264 should tolerate this pretty well, except maybe 50D) */
        return CBR_RET_CONTINUE;
    }
    
    if (lv_dirty)
    {
        /* do we need to refresh LiveView? */
        if (crop_rec_needs_lv_refresh())
        {
            /* let's check this once again, just in case */
            /* (possible race condition that would result in unnecessary refresh) */
            msleep(500);
            if (crop_rec_needs_lv_refresh())
            {
                info_led_on();
                gui_uilock(UILOCK_EVERYTHING);
                int old_zoom = lv_dispsize;
                set_zoom(lv_dispsize == 1 ? 5 : 1);
                set_zoom(old_zoom);
                gui_uilock(UILOCK_NONE);
                info_led_off();
            }
        }
        lv_dirty = 0;
    }
    
    if (crop_preset == CROP_PRESET_CENTER_Z &&
        (lv_dispsize == 5 || lv_dispsize == 10))
    {
        center_canon_preview();
    }
    
    /* mv720p modes 5D3. Needs refreshing if iterating through x5,x10 mode */
    
    static int patch = 0;
    
    if (((crop_preset == CROP_PRESET_3x3_1X || crop_preset == CROP_PRESET_3x3_1X_50p ||
          crop_preset == CROP_PRESET_3x3_1X_60p || crop_preset == CROP_PRESET_3x3_1X_48p ||
          crop_preset == CROP_PRESET_3x3_mv1080_48fps_EOSM || crop_preset == CROP_PRESET_3x3_1X_100D ||
          crop_preset == CROP_PRESET_3x3_1X_45p)) && lv_dispsize == 5)
    {
        /* workaround for x5,x10 back to mv1080p mode without freezing */
        patch = 1;
    }
    
    if (((crop_preset == CROP_PRESET_3x3_1X || crop_preset == CROP_PRESET_3x3_1X_50p ||
          crop_preset == CROP_PRESET_3x3_1X_60p || crop_preset == CROP_PRESET_3x3_1X_48p ||
          crop_preset == CROP_PRESET_3x3_mv1080_48fps_EOSM || crop_preset == CROP_PRESET_3x3_1X_100D ||
          crop_preset == CROP_PRESET_3x3_1X_45p) && patch) && lv_dispsize == 1)
    {
        patch = 0;
        patch_active = 0;
        
        info_led_on();
        gui_uilock(UILOCK_EVERYTHING);
        int old_zoom = lv_dispsize;
        set_zoom(lv_dispsize == 1 ? 5 : 1);
        set_zoom(old_zoom);
        gui_uilock(UILOCK_NONE);
        info_led_off();
    }
    
    static bool once = false;
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_100D || CROP_PRESET_MENU == CROP_PRESET_4K_EOSM)
    {
        if (once == false)
        {
            once = true;
            NotifyBox(4000, "Turn audio OFF if not done already!");
        }
    }
    else
    {
        once = false;
    }
    
    if (!crop_patch2 && get_halfshutter_pressed() && (zoomaid == 0x1 || zoomaid == 0x2) && !is_6D && !is_5D3)
    {
        
        /* dark mode */
        if (zoomaid == 0x2) NotifyBox(3000, "dark mode");
        
        /* zoomaid */
        if (get_halfshutter_pressed() && (zoomaid == 0x1 || zoomaid == 0x2))
        {
            
            /* disable for now. Not working the same as for non rewired mode */
            if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM && zoomaid == 0x0)
            {
                return 0;
            }
            
            crop_patch2 = 1;
            if (CROP_PRESET_MENU != CROP_PRESET_anamorphic_rewired_EOSM && CROP_PRESET_MENU != CROP_PRESET_mcm_mv1080_EOSM &&
                CROP_PRESET_MENU != CROP_PRESET_anamorphic_rewired_100D)
            {
                if (CROP_PRESET_MENU != CROP_PRESET_CENTER_Z_EOSM &&
                    CROP_PRESET_MENU != CROP_PRESET_3x3_1X_EOSM &&
                    CROP_PRESET_MENU != CROP_PRESET_2K_EOSM &&
                    CROP_PRESET_MENU != CROP_PRESET_3K_EOSM &&
                    CROP_PRESET_MENU != CROP_PRESET_4K_EOSM)
                {
                    info_led_on();
                    gui_uilock(UILOCK_EVERYTHING);
                    int old_zoom = lv_dispsize;
                    set_zoom(lv_dispsize == 1 ? 5 : 1);
                    set_zoom(old_zoom);
                    gui_uilock(UILOCK_NONE);
                    info_led_off();
                }
                if ((zoomaid == 0x1 || zoomaid == 0x2)) set_lv_zoom(10);
            }
            else
            {
                if (crop_preset == CROP_PRESET_x10_EOSM) movie_crop_hack_disable();
                /* fixes interference with autoiso(replacing PauseLiveView();) */
                display_off();
                msleep(300);
                display_on();
                ResumeLiveView();
                if (zoomaid == 0x1 || zoomaid == 0x2) set_lv_zoom(10);
            }
            while (get_halfshutter_pressed())
            {
                msleep(10);
            }
        }
        
        if (!get_halfshutter_pressed() && crop_patch2)
        {
            crop_patch2 = 0;
            if (CROP_PRESET_MENU != CROP_PRESET_anamorphic_rewired_EOSM && CROP_PRESET_MENU != CROP_PRESET_mcm_mv1080_EOSM &&
                CROP_PRESET_MENU != CROP_PRESET_anamorphic_rewired_100D)
            {
                if (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_EOSM ||
                    CROP_PRESET_MENU == CROP_PRESET_3x3_1X_EOSM ||
                    CROP_PRESET_MENU == CROP_PRESET_2K_EOSM ||
                    CROP_PRESET_MENU == CROP_PRESET_3K_EOSM ||
                    CROP_PRESET_MENU == CROP_PRESET_4K_EOSM)
                {
                    set_lv_zoom(5);
                }
                else
                {
                    info_led_on();
                    gui_uilock(UILOCK_EVERYTHING);
                    int old_zoom = lv_dispsize;
                    set_zoom(lv_dispsize == 1 ? 5 : 1);
                    set_zoom(old_zoom);
                    gui_uilock(UILOCK_NONE);
                    info_led_off();
                    set_lv_zoom(1);
                }
            }
            else
            {
                if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM || CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM ||
                    CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D) movie_crop_hack_enable();
                PauseLiveView();
                ResumeLiveView();
            }
            if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D)
            /* 100D is a stubborn thing, needs an extra round */
            {
                PauseLiveView();
                ResumeLiveView();
            }
        }
    }
    
    
    if (x3toggle != 0x1 || x3toggle != 0x2 || zoomaid != 0x0) crop_patch = 0; //disable patch while off
    
    if (((CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_EOSM) ||
         (CROP_PRESET_MENU == CROP_PRESET_2K_100D) ||
         (CROP_PRESET_MENU == CROP_PRESET_3K_100D) ||
         (CROP_PRESET_MENU == CROP_PRESET_4K_100D) ||
         (CROP_PRESET_MENU == CROP_PRESET_2K_EOSM) ||
         (CROP_PRESET_MENU == CROP_PRESET_3K_EOSM) ||
         (CROP_PRESET_MENU == CROP_PRESET_4K_EOSM) ||
         (CROP_PRESET_MENU == CROP_PRESET_4K_3x1_EOSM) ||
         (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_EOSM) ||
         (CROP_PRESET_MENU == CROP_PRESET_4K_3x1_100D) ||
         (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_100D) ||
         (CROP_PRESET_MENU == CROP_PRESET_1080K_100D) ||
         (CROP_PRESET_MENU == CROP_PRESET_2520_1418_700D) ||
         (CROP_PRESET_MENU == CROP_PRESET_3K_700D) ||
         (CROP_PRESET_MENU == CROP_PRESET_3540_700D) ||
         (CROP_PRESET_MENU == CROP_PRESET_4K_700D) ||
         (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_700D) ||
         (CROP_PRESET_MENU == CROP_PRESET_2520_1418_650D) ||
         (CROP_PRESET_MENU == CROP_PRESET_3K_650D) ||
         (CROP_PRESET_MENU == CROP_PRESET_3540_650D) ||
         (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z_650D) ||
         (CROP_PRESET_MENU == CROP_PRESET_4K_650D) ||
         (CROP_PRESET_MENU == CROP_PRESET_CENTER_Z)) && lv_dispsize == 1)
    {
        info_led_on();
        gui_uilock(UILOCK_EVERYTHING);
        int old_zoom = lv_dispsize;
        set_zoom(lv_dispsize == 1 ? 5 : 1);
        set_zoom(old_zoom);
        gui_uilock(UILOCK_NONE);
        info_led_off();
        set_lv_zoom(5);
    }
    return CBR_RET_CONTINUE;
}

/* Display recording status in top info bar */
static LVINFO_UPDATE_FUNC(crop_info)
{
    LVINFO_BUFFER(16);
    
    if (patch_active)
    {
        if (lv_dispsize > 1)
        {
            switch (crop_preset)
            {
                case CROP_PRESET_CENTER_Z:
                    snprintf(buffer, sizeof(buffer), "3.5K");
                    break;
            }
        }
    }
    
    /* 5D3 */
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_1X || CROP_PRESET_MENU == CROP_PRESET_3x3_1X_50p || CROP_PRESET_MENU == CROP_PRESET_3x3_1X_60p)
    {
        snprintf(buffer, sizeof(buffer), "3x3 mv1080p");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_3X)
    {
        snprintf(buffer, sizeof(buffer), "1x1 mv1080p");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_3X_TALL)
    {
        snprintf(buffer, sizeof(buffer), "3x_Tall");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_1X_48p)
    {
        snprintf(buffer, sizeof(buffer), "mv1050p 48fps");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_1X_45p)
    {
        snprintf(buffer, sizeof(buffer), "mv1080p 45fps");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_3K)
    {
        snprintf(buffer, sizeof(buffer), "3K");
    }
    
    if (CROP_PRESET_MENU ==CROP_PRESET_UHD)
    {
        snprintf(buffer, sizeof(buffer), "UHD");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_mv1080_mv720p)
    {
        snprintf(buffer, sizeof(buffer), "passthrough");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_1x3)
    {
        if (ratios == 0x1)
        {
            snprintf(buffer, sizeof(buffer), "anamorph 2.39:1");
        }
        if (ratios == 0x2)
        {
            snprintf(buffer, sizeof(buffer), "anamorph 2.35:1");
        }
        if (ratios == 0x3)
        {
            snprintf(buffer, sizeof(buffer), "anamorph 16:9");
        }
        if (ratios == 0x0)
        {
            snprintf(buffer, sizeof(buffer), "anamorphic");
        }
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_1x3_17fps)
    {
        snprintf(buffer, sizeof(buffer), "1x3 mv1080p");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_FULLRES_LV)
    {
        snprintf(buffer, sizeof(buffer), "fullres");
    }
    
    
    /* 100D */
    if (CROP_PRESET_MENU == CROP_PRESET_mv1080p_mv720p_100D)
    {
        snprintf(buffer, sizeof(buffer), "mv1080p_mv720p mode");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_3xcropmode_100D)
    {
        snprintf(buffer, sizeof(buffer), "MovieCropMode");
    }
    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_100D)
    {
        snprintf(buffer, sizeof(buffer), "anamorphic");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_1080K_100D)
    {
        snprintf(buffer, sizeof(buffer), "2520x1080");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_2K_100D)
    {
        snprintf(buffer, sizeof(buffer), "2520x1418");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3K_100D)
    {
        snprintf(buffer, sizeof(buffer), "3000x1432");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_100D)
    {
        snprintf(buffer, sizeof(buffer), "4080x3000");
        if (ratios == 0x1 && timelapse == 0x0) snprintf(buffer, sizeof(buffer), "2.39:1");
        if (ratios == 0x2 && timelapse == 0x0) snprintf(buffer, sizeof(buffer), "2.35:1");
        if (ratios == 0x3 && timelapse == 0x0) snprintf(buffer, sizeof(buffer), "16:9");
        if (timelapse == 0x1 || timelapse == 0x7) snprintf(buffer, sizeof(buffer), "timelapse0.4fps");
        if (timelapse == 0x2 || timelapse == 0x8) snprintf(buffer, sizeof(buffer), "timelapse 1fps");
        if (timelapse == 0x3 || timelapse == 0x9) snprintf(buffer, sizeof(buffer), "timelapse 2fps");
        if (timelapse == 0x4) snprintf(buffer, sizeof(buffer), "timelapse 3fps");
        if (timelapse == 0x5) snprintf(buffer, sizeof(buffer), "timelapse 4fps");
        if (timelapse == 0x6) snprintf(buffer, sizeof(buffer), "timelapse 5fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_1X_100D)
    {
        snprintf(buffer, sizeof(buffer), "3x3 720p");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_rewired_EOSM)
    {
        if (ratios == 0x1)
        {
            snprintf(buffer, sizeof(buffer), "anamorph 2.39:1");
        }
        if (ratios == 0x2)
        {
            snprintf(buffer, sizeof(buffer), "anamorph 2.35:1");
        }
        if (ratios == 0x3)
        {
            snprintf(buffer, sizeof(buffer), "anamorph 16:9");
        }
    }
    
    /* EOSM */
    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "5K anamorphic");
        if (ratios == 0x1 || ratios == 0x2)
        {
            snprintf(buffer, sizeof(buffer), "4.5K anamorphic");
        }
        if (ratios == 0x3)
        {
            snprintf(buffer, sizeof(buffer), "3.5K anamorphic");
        }
        
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_2K_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "2520x1418");
        if (ratios == 0x2)
        {
            snprintf(buffer, sizeof(buffer), "2K");
        }
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3K_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "3K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "4080x3000");
        if (ratios == 0x1 && timelapse == 0x0) snprintf(buffer, sizeof(buffer), "2.39:1");
        if (ratios == 0x2 && timelapse == 0x0) snprintf(buffer, sizeof(buffer), "2.35:1");
        if (ratios == 0x3 && timelapse == 0x0) snprintf(buffer, sizeof(buffer), "16:9");
        if (timelapse == 0x1 || timelapse == 0x7) snprintf(buffer, sizeof(buffer), "timelapse0.4fps");
        if (timelapse == 0x2 || timelapse == 0x8) snprintf(buffer, sizeof(buffer), "timelapse 1fps");
        if (timelapse == 0x3 || timelapse == 0x9) snprintf(buffer, sizeof(buffer), "timelapse 2fps");
        if (timelapse == 0x4) snprintf(buffer, sizeof(buffer), "timelapse 3fps");
        if (timelapse == 0x5) snprintf(buffer, sizeof(buffer), "timelapse 4fps");
        if (timelapse == 0x6) snprintf(buffer, sizeof(buffer), "timelapse 5fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_3x1_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "4K 3x1 24fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "5K 3x1 24fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_3x1_100D)
    {
        snprintf(buffer, sizeof(buffer), "4K 3x1 24fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_5K_3x1_100D)
    {
        snprintf(buffer, sizeof(buffer), "5K 3x1 24fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_5x1_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "4K 5x1 24fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "mv1080p");
        if (x3crop == 0x1)
        {
            snprintf(buffer, sizeof(buffer), "mv1080p x3zoom");
        }
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_mcm_mv1080_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "mv1080p rewire");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_EOSM || CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_700D || CROP_PRESET_MENU == CROP_PRESET_3x3_mv1080_48fps_650D)
    {
        if (ratios == 0x0) snprintf(buffer, sizeof(buffer), "mv1080p_46fps");
        if (ratios == 0x1 || ratios == 0x2) snprintf(buffer, sizeof(buffer), "mv1080p_48fps");
        if (ratios == 0x3) snprintf(buffer, sizeof(buffer), "mv1080p_45fps");
        if ((ratios == 0x1 || ratios == 0x2) && set_25fps == 0x1) snprintf(buffer, sizeof(buffer), "mv1080p_50fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3x1_mv720_50fps_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "mv720p_50fps");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_3x3_1X_EOSM)
    {
        snprintf(buffer, sizeof(buffer), "3x3 720p");
    }
    
    /* 700D */
    if (CROP_PRESET_MENU == CROP_PRESET_3K_700D)
    {
        snprintf(buffer, sizeof(buffer), "3K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_700D)
    {
        snprintf(buffer, sizeof(buffer), "4K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_UHD_700D)
    {
        snprintf(buffer, sizeof(buffer), "UHD");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_FULLRES_LV_700D)
    {
        snprintf(buffer, sizeof(buffer), "FLV");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_2520_1418_700D)
    {
        snprintf(buffer, sizeof(buffer), "2K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_700D)
    {
        snprintf(buffer, sizeof(buffer), "5K anamorphic");
    }
    
    /* 650D */
    if (CROP_PRESET_MENU == CROP_PRESET_3K_650D)
    {
        snprintf(buffer, sizeof(buffer), "3K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_650D)
    {
        snprintf(buffer, sizeof(buffer), "4K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_UHD_650D)
    {
        snprintf(buffer, sizeof(buffer), "UHD");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_FULLRES_LV_650D)
    {
        snprintf(buffer, sizeof(buffer), "FLV");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_2520_1418_650D)
    {
        snprintf(buffer, sizeof(buffer), "2K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_anamorphic_650D)
    {
        snprintf(buffer, sizeof(buffer), "5K anamorphic");
    }
    
    /* 6D */
    if (CROP_PRESET_MENU == CROP_PRESET_HD100_6D)
    {
        snprintf(buffer, sizeof(buffer), "HD100");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_HD75_6D)
    {
        snprintf(buffer, sizeof(buffer), "HD75");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_HD50_6D)
    {
        snprintf(buffer, sizeof(buffer), "HD50");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_HD2_6D)
    {
        snprintf(buffer, sizeof(buffer), "HD2");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_1x3_6D)
    {
        snprintf(buffer, sizeof(buffer), "1x3");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_2K_6D)
    {
        snprintf(buffer, sizeof(buffer), "2K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_ULTRA_HD_6D)
    {
        snprintf(buffer, sizeof(buffer), "ULTRA HD");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_4K_6D)
    {
        snprintf(buffer, sizeof(buffer), "4K");
    }
    
    if (CROP_PRESET_MENU == CROP_PRESET_5K_6D)
    {
        snprintf(buffer, sizeof(buffer), "5K");
    }
    
    
    if (CROP_PRESET_MENU == CROP_PRESET_FullRes_6D)
    {
        snprintf(buffer, sizeof(buffer), "FullRes");
    }
    
    /* append info about current binning mode */
    
    if (raw_lv_is_enabled())
    {
        /* fixme: raw_capture_info is only updated when LV RAW is active */
        
        STR_APPEND("%s%dx%d",
                   buffer[0] ? " " : "",
                   raw_capture_info.binning_x + raw_capture_info.skipping_x,
                   raw_capture_info.binning_y + raw_capture_info.skipping_y
                   );
    }
    
}

static struct lvinfo_item info_items[] = {
    {
        .name = "Crop info",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = crop_info,
        .preferred_position = -50,  /* near the focal length display */
        .priority = 1,
    }
};

static unsigned int raw_info_update_cbr(unsigned int unused)
{
    if (patch_active)
    {
        /* not implemented yet */
        raw_capture_info.offset_x = raw_capture_info.offset_y   = SHRT_MIN;
        
        if ((lv_dispsize > 1) && (!is_EOSM))
        {
            /* raw backend gets it right */
            return 0;
        }
        
        /* update horizontal pixel binning parameters */
        switch (crop_preset)
        {
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3K:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_UHD:
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_3x1:
            case CROP_PRESET_FULLRES_LV_700D:
            case CROP_PRESET_FULLRES_LV_650D:
            case CROP_PRESET_3xcropmode_100D:
                raw_capture_info.binning_x    = raw_capture_info.binning_y  = 1;
                raw_capture_info.skipping_x   = raw_capture_info.skipping_y = 0;
                break;
                
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_50p:
            case CROP_PRESET_3x3_1X_60p:
            case CROP_PRESET_3x3_1X_100D:
            case CROP_PRESET_3x3_1X_EOSM:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_3x3_1X_45p:
            case CROP_PRESET_1x3:
            case CROP_PRESET_1x3_17fps:
            case CROP_PRESET_3x3_mv1080_EOSM:
            case CROP_PRESET_3x3_mv1080_700D:
            case CROP_PRESET_3x3_mv1080_650D:
            case CROP_PRESET_mcm_mv1080_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_700D:
            case CROP_PRESET_3x3_mv1080_48fps_650D:
            case CROP_PRESET_anamorphic_rewired_EOSM:
            case CROP_PRESET_anamorphic_EOSM:
            case CROP_PRESET_anamorphic_700D:
            case CROP_PRESET_anamorphic_650D:
            case CROP_PRESET_anamorphic_rewired_100D:
                raw_capture_info.binning_x = 3; raw_capture_info.skipping_x = 0;
                break;
                
            case CROP_PRESET_4K_3x1_EOSM:
            case CROP_PRESET_5K_3x1_EOSM:
            case CROP_PRESET_4K_3x1_100D:
            case CROP_PRESET_5K_3x1_100D:
                if (lv_dispsize == 1)
                {
                    raw_capture_info.binning_x = 3; raw_capture_info.skipping_x = 0;
                    raw_capture_info.binning_y = 1; raw_capture_info.skipping_y = 0;
                }
                else
                {
                    raw_capture_info.binning_x = 1; raw_capture_info.skipping_x = 0;
                    raw_capture_info.binning_y = 3; raw_capture_info.skipping_y = 0;
                }
                break;
                
            case CROP_PRESET_4K_5x1_EOSM:
                raw_capture_info.binning_x = 1; raw_capture_info.skipping_x = 0;
                raw_capture_info.binning_y = 5; raw_capture_info.skipping_y = 0;
                break;
                
            case CROP_PRESET_HD100_6D:
            case CROP_PRESET_HD75_6D:
            case CROP_PRESET_HD50_6D:
            case CROP_PRESET_1x3_6D:
            case CROP_PRESET_1x3_24FPS_6D:
            case CROP_PRESET_2K_6D:
            case CROP_PRESET_ULTRA_HD_6D:
            case CROP_PRESET_4K_6D:
            case CROP_PRESET_FullRes_6D:
            case CROP_PRESET_HD2_6D:
                raw_capture_info.binning_x    = raw_capture_info.binning_y  = 1;
                raw_capture_info.skipping_x   = raw_capture_info.skipping_y = 0;
                break;
        }
        
        /* update vertical pixel binning / line skipping parameters */
        switch (crop_preset)
        {
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3K:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_UHD:
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_FULLRES_LV_700D:
            case CROP_PRESET_FULLRES_LV_650D:
            case CROP_PRESET_1x3:
            case CROP_PRESET_1x3_17fps:
            case CROP_PRESET_anamorphic_rewired_EOSM:
            case CROP_PRESET_anamorphic_EOSM:
            case CROP_PRESET_anamorphic_700D:
            case CROP_PRESET_anamorphic_650D:
            case CROP_PRESET_anamorphic_rewired_100D:
            case CROP_PRESET_3xcropmode_100D:
                raw_capture_info.binning_y = 1; raw_capture_info.skipping_y = 0;
                break;
                
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_50p:
            case CROP_PRESET_3x3_1X_60p:
            case CROP_PRESET_3x3_1X_100D:
            case CROP_PRESET_3x3_1X_EOSM:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_3x3_1X_45p:
            case CROP_PRESET_3x1:
            case CROP_PRESET_3x3_mv1080_EOSM:
            case CROP_PRESET_3x3_mv1080_700D:
            case CROP_PRESET_3x3_mv1080_650D:
            case CROP_PRESET_mcm_mv1080_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_EOSM:
            case CROP_PRESET_3x3_mv1080_48fps_700D:
            case CROP_PRESET_3x3_mv1080_48fps_650D:
            case CROP_PRESET_HD100_6D:
            case CROP_PRESET_HD75_6D:
            case CROP_PRESET_HD50_6D:
            case CROP_PRESET_1x3_6D:
            case CROP_PRESET_1x3_24FPS_6D:
            case CROP_PRESET_2K_6D:
            case CROP_PRESET_ULTRA_HD_6D:
            case CROP_PRESET_4K_6D:
            case CROP_PRESET_FullRes_6D:
            case CROP_PRESET_HD2_6D:
            {
                int b = (is_5D3) ? 3 : 1;
                int s = (is_5D3) ? 0 : 2;
                raw_capture_info.binning_y = b; raw_capture_info.skipping_y = s;
                break;
            }
        }
        
        if ((is_5D3) || (is_EOSM) || (is_100D) || (is_700D) || (is_650D) || (is_6D))
        {
            /* update skip offsets */
            int skip_left, skip_right, skip_top, skip_bottom;
            calc_skip_offsets(&skip_left, &skip_right, &skip_top, &skip_bottom);
            raw_set_geometry(raw_info.width, raw_info.height, skip_left, skip_right, skip_top, skip_bottom);
        }
    }
    
    return 0;
}

static unsigned int crop_rec_init()
{
    is_digic4 = is_camera("DIGIC", "4");
    is_digic5 = is_camera("DIGIC", "5");
    
    if (is_camera("5D3",  "1.1.3") || is_camera("5D3", "1.2.3"))
    {
        /* same addresses on both 1.1.3 and 1.2.3 */
        CMOS_WRITE = 0x119CC;
        MEM_CMOS_WRITE = 0xE92D47F0;
        
        ADTG_WRITE = 0x11640;
        MEM_ADTG_WRITE = 0xE92D47F0;
        
        ENGIO_WRITE = is_camera("5D3", "1.2.3") ? 0xFF290F98 : 0xFF28CC3C;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_5D3 = 1;
        crop_presets                = crop_presets_5d3;
        crop_rec_menu[0].choices    = crop_choices_5d3;
        crop_rec_menu[0].max        = COUNT(crop_choices_5d3) - 1;
        crop_rec_menu[0].help       = crop_choices_help_5d3;
        crop_rec_menu[0].help2      = crop_choices_help2_5d3;
        
        fps_main_clock = 24000000;
        /* 24p,  25p,  30p,  50p,  60p,   x5 */
        memcpy(default_timerA, (int[]) {  440,  480,  440,  480,  440,  518 }, 24);
        memcpy(default_timerB, (int[]) { 2275, 2000, 1820, 1000,  910, 1556 }, 24);
    }
    else if (is_camera("EOSM", "2.0.2"))
    {
        CMOS_WRITE = 0x2998C;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x2986C;
        MEM_ADTG_WRITE = 0xE92D43F8;
        
        ENGIO_WRITE = 0xFF2C19AC;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_EOSM = 1;
        crop_presets                = crop_presets_eosm;
        crop_rec_menu[0].choices    = crop_choices_eosm;
        crop_rec_menu[0].max        = COUNT(crop_choices_eosm) - 1;
        crop_rec_menu[0].help       = crop_choices_help_eosm;
        crop_rec_menu[0].help2      = crop_choices_help2_eosm;
    }
    else if (is_camera("700D", "1.1.5"))
    {
        CMOS_WRITE = 0x17A1C;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x178FC;
        MEM_ADTG_WRITE = 0xE92D43F8;
        
        ENGIO_WRITE = 0xFF2C2D00;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_700D = 1;
        crop_presets                = crop_presets_700d;
        crop_rec_menu[0].choices    = crop_choices_700d;
        crop_rec_menu[0].max        = COUNT(crop_choices_700d) - 1;
        crop_rec_menu[0].help       = crop_choices_help_700d;
        crop_rec_menu[0].help2      = crop_choices_help2_700d;
    }
    else if (is_camera("650D", "1.0.4"))
    {
        CMOS_WRITE = 0x17A1C;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x178FC;
        MEM_ADTG_WRITE = 0xE92D43F8;
        
        ENGIO_WRITE = 0xFF2C0778;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_650D = 1;
        crop_presets                = crop_presets_650d;
        crop_rec_menu[0].choices    = crop_choices_650d;
        crop_rec_menu[0].max        = COUNT(crop_choices_650d) - 1;
        crop_rec_menu[0].help       = crop_choices_help_650d;
        crop_rec_menu[0].help2      = crop_choices_help2_650d;
    }
    else if (is_camera("100D", "1.0.1"))
    {
        CMOS_WRITE = 0x475B8;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x47144;
        MEM_ADTG_WRITE = 0xE92D43F8;
        
        ENGIO_WRITE = 0xFF2B2460;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_100D = 1;
        crop_presets                = crop_presets_100d;
        crop_rec_menu[0].choices    = crop_choices_100d;
        crop_rec_menu[0].max        = COUNT(crop_choices_100d) - 1;
        crop_rec_menu[0].help       = crop_choices_help_100d;
        crop_rec_menu[0].help2      = crop_choices_help2_100d;
    }
    else if (is_camera("70D", "1.1.2"))
    {
        CMOS_WRITE = 0x26B54;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x2684C;
        MEM_ADTG_WRITE = 0xE92D47F0;
        
        is_basic = 1;
        crop_presets                = crop_presets_basic;
        crop_rec_menu[0].choices    = crop_choices_basic;
        crop_rec_menu[0].max        = COUNT(crop_choices_basic) - 1;
        crop_rec_menu[0].help       = crop_choices_help_basic;
        crop_rec_menu[0].help2      = crop_choices_help2_basic;
    }
    else if (is_camera("6D", "1.1.6"))
    {
        CMOS_WRITE = 0x2420C;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x24108;
        MEM_ADTG_WRITE = 0xE92D41F0;
        
        ENGIO_WRITE = 0xFF2AE134;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_6D = 1;
        crop_presets                = crop_presets_6d;
        crop_rec_menu[0].choices    = crop_choices_6d;
        crop_rec_menu[0].max        = COUNT(crop_choices_6d) - 1;
        crop_rec_menu[0].help       = crop_choices_help_6d;
        crop_rec_menu[0].help2      = crop_choices_help2_6d;
        
        fps_main_clock = 25600000;
        /* 24p,  25p,  30p,  50p,  60p,   x5 */
        memcpy(default_timerA, (int[]) {  546,  640,  546,  640,  520,  730 }, 24);
        memcpy(default_timerB, (int[]) { 1955, 1600, 1564,  800,  821, 1172 }, 24);
        /* or 1956        1565         822        2445        1956 */
    }
    
    /* default FPS timers are the same on all these models */
    if (is_EOSM || is_700D || is_650D || is_100D)
    {
        fps_main_clock = 32000000;
        /* 24p,  25p,  30p,  50p,  60p,   x5, c24p, c25p, c30p */
        memcpy(default_timerA, (int[]) {  528,  640,  528,  640,  528,  716,  546,  640,  546 }, 36);
        memcpy(default_timerB, (int[]) { 2527, 2000, 2022, 1000, 1011, 1491, 2444, 2000, 1955 }, 36);
        /* or 2528        2023        1012        2445        1956 */
    }
    
    /* FPS in x5 zoom may be model-dependent; assume exact */
    default_fps_1k[5] = (uint64_t) fps_main_clock * 1000ULL / default_timerA[5] / default_timerB[5];
    
    printf("[crop_rec] checking FPS timer values...\n");
    for (int i = 0; i < COUNT(default_fps_1k); i++)
    {
        if (default_timerA[i])
        {
            int fps_i = (uint64_t) fps_main_clock * 1000ULL / default_timerA[i] / default_timerB[i];
            if (fps_i == default_fps_1k[i])
            {
                printf("%d) %s%d.%03d: A=%d B=%d (exact)\n", i, FMT_FIXEDPOINT3(default_fps_1k[i]), default_timerA[i], default_timerB[i]);
                
                if (i == 5 && default_fps_1k[i] != 29970)
                {
                    printf("-> unusual FPS in x5 zoom\n", i);
                }
            }
            else
            {
                int fps_p = (uint64_t) fps_main_clock * 1000ULL / default_timerA[i] / (default_timerB[i] + 1);
                if (fps_i > default_fps_1k[i] && fps_p < default_fps_1k[i])
                {
                    printf("%d) %s%d.%03d: A=%d B=%d/%d (averaged)\n", i, FMT_FIXEDPOINT3(default_fps_1k[i]), default_timerA[i], default_timerB[i], default_timerB[i] + 1);
                }
                else
                {
                    printf("%d) %s%d.%03d: A=%d B=%d (%s%d.%03d ?!?)\n", i, FMT_FIXEDPOINT3(default_fps_1k[i]), default_timerA[i], default_timerB[i], FMT_FIXEDPOINT3(fps_i));
                    return CBR_RET_ERROR;
                }
                
                /* assume 25p is exact on all models */
                if (i == 1)
                {
                    printf("-> 25p check error\n");
                    return CBR_RET_ERROR;
                }
            }
        }
    }
    
    menu_add("Movie", crop_rec_menu, COUNT(crop_rec_menu));
    lvinfo_add_items (info_items, COUNT(info_items));
    
    return 0;
}

static unsigned int crop_rec_deinit()
{
    return 0;
}

MODULE_INFO_START()
MODULE_INIT(crop_rec_init)
MODULE_DEINIT(crop_rec_deinit)
MODULE_INFO_END()

MODULE_CONFIGS_START()
MODULE_CONFIG(crop_preset_index)
MODULE_CONFIG(shutter_range)
MODULE_CONFIG(bitdepth)
MODULE_CONFIG(ratios)
MODULE_CONFIG(x3crop)
MODULE_CONFIG(set_25fps)
MODULE_CONFIG(HDR_iso_a)
MODULE_CONFIG(HDR_iso_b)
MODULE_CONFIG(isoauto)
MODULE_CONFIG(isoclimb)
MODULE_CONFIG(timelapse)
MODULE_CONFIG(slowshutter)
MODULE_CONFIG(x3toggle)
MODULE_CONFIG(zoomaid)
MODULE_CONFIGS_END()

MODULE_CBRS_START()
MODULE_CBR(CBR_SHOOT_TASK, crop_rec_polling_cbr, 0)
MODULE_CBR(CBR_RAW_INFO_UPDATE, raw_info_update_cbr, 0)
MODULE_CBR(CBR_KEYPRESS, crop_rec_keypress_cbr, 0)
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLER(PROP_LV_ACTION)
MODULE_PROPHANDLER(PROP_LV_DISPSIZE)
MODULE_PROPHANDLERS_END()
