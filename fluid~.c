// Original version by Frank Barknecht (fbar@footils.org) 2003
// Ported from Flext/C++ to plain C/pdlibbuilder by Jonathan Wilkes 2016 for Purr Data
// SMMF mode and various other little improvements by Albert Gräf 2020
// Revised by Porres and ported to distribute for Pd Vanilla 2021
// Distributed under the GPLv2+, please check the LICENSE file for details.

#include "m_pd.h"
#include <fluidsynth.h>
#include <string.h>
#include <unistd.h>
 
static t_class *fluid_tilde_class;
 
typedef struct _fluid_tilde{
    t_object x_obj;
    fluid_synth_t *x_synth;
    fluid_settings_t *x_settings;
    t_outlet *x_out_left;
    t_outlet *x_out_right;
    t_canvas *x_canvas;
    int smmf_mode;
}t_fluid_tilde;

t_int *fluid_tilde_perform(t_int *w){
    t_fluid_tilde *x = (t_fluid_tilde *)(w[1]);
    t_sample *left = (t_sample *)(w[2]);
    t_sample *right = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    fluid_synth_write_float(x->x_synth, n, left, 0, 1, right, 0, 1);
    return(w+5);
}

static void fluid_tilde_dsp(t_fluid_tilde *x, t_signal **sp){
    dsp_add(fluid_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void fluid_tilde_free(t_fluid_tilde *x){
    if(x->x_synth)
        delete_fluid_synth(x->x_synth);
    if(x->x_settings)
        delete_fluid_settings(x->x_settings);
}

static void fluid_help(void){
    post("\n");
    post("---------------------------------------------------------");
    post(" - [fluid~] 2.0: A soundfont external for Pure Data");
    post(" based on fluidsynth (https://www.fluidsynth.org)");
    post(" - fluidsynth version: %s ", FLUIDSYNTH_VERSION);
    post("---------------------------------------------------------");
    post("- Initialization arguments:");
    post("   (optional) -smmf: enables SMMF mode");
    post("   any other symbol: soundfont file to load on object creation");
    post("---------------------------------------------------------");
    post("- MESSAGES:");
    post("load <symbol>: - Load soundfont file");
    post("---------------------------------------------------------");
    post("- LEGACY mode's Control Messages:");
    post("note <float, float, float>: - Play note <channel, note, velocity>");
    post("n <float, float, float>: - Same as above (play note)");
    post("list <float, float, float>: - Same as above (play note)");
    post("bend <float, float>: - Bend notes <channel, value>");
    post("b <float, float>: - Bend notes <channel, value>");
    post("control <float, float, float>: - Control change <channel, control, value>");
    post("cc <float, float, float>: - Control change <channel, control, value>");
    post("c <float, float, float>: - Control change <channel, control, value>");
    post("prog <float, float, float>: - Progam change <channel, program>");
    post("p <float, float, float>: - Progam change <channel, program>");
    post("bank <float, float, float>: - Bank change <channel, bank>");
    post("---------------------------------------------------------");
    post("- SMMF mode's Control Messages:");
    post("note <float, float, float>: - Play note <note, velocity, channel>");
    post("bend <float, float>: - Bend notes <value, channel>");
    post("pgm <float, float>: - Progam change <program, channel>");
    post("ctl <float, float>: - Control change <control, value, channel>");
    post("touch <float, float>: - Channel aftertouch <value, channel>");
    post("polytouch <float, float, float>: - Key aftertouch <key, value, channel>");
    post("sysex <list>: - Sysex messages");
    post("---------------------------------------------------------");
    post("\n");
}

static void fluid_legacy_note(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(argc == 3){
        int chan, key, vel;
        chan = atom_getintarg(0, argc, argv);
        key = atom_getintarg(1, argc, argv);
        vel = atom_getintarg(2, argc, argv);
        fluid_synth_noteon(x->x_synth, chan - 1, key, vel);
    }
}

static void fluid_legacy_program_change(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(argc == 2){
        int chan, prog;
        chan = atom_getintarg(0, argc, argv);
        prog = atom_getintarg(1, argc, argv);
        fluid_synth_program_change(x->x_synth, chan-1, prog);
    }
}

static void fluid_legacy_control_change(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(argc == 3){
        int chan, ctrl, val;
        chan = atom_getintarg(0, argc, argv);
        ctrl = atom_getintarg(1, argc, argv);
        val = atom_getintarg(2, argc, argv);
        fluid_synth_cc(x->x_synth, chan-1, ctrl, val);
    }
}

static void fluid_legacy_pitch_bend(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(argc == 2){
        int chan, val;
        chan = atom_getintarg(0, argc, argv);
        val = atom_getintarg(1, argc, argv);
        fluid_synth_pitch_bend(x->x_synth, chan-1, val);
    }
}

static void fluid_legacy_bank(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(argc == 2){
        int chan, bank;
        chan = atom_getintarg(0, argc, argv);
        bank = atom_getintarg(1, argc, argv);
        fluid_synth_bank_select(x->x_synth, chan-1, bank);
    }
}

static void fluid_legacy_gen(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(argc == 3){
        int chan, param;
        float value;
        chan = atom_getintarg(0, argc, argv);
        param = atom_getintarg(1, argc, argv);
        value = atom_getintarg(2, argc, argv);
        fluid_synth_set_gen(x->x_synth, chan-1, param, value);
    }
}

// Note that in all the SMMF methods we allow the channel to be omitted, in
// which case it defaults to 1. Also note that the channel argument *always*
// comes last, and that the argument order, being in 1-1 correspondence with
// the Pd MIDI objects, is a bit different from the legacy message format
// above which follows the MIDI standard instead.

// Please check https://bitbucket.org/agraef/pd-smmf for details.

static void fluid_note(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(!x->smmf_mode){
        fluid_legacy_note(x, s, argc, argv);
        return;
    }
    if(argc == 2 || argc == 3){
        int key = atom_getintarg(0, argc, argv);
        int vel = atom_getintarg(1, argc, argv);
        int chan = argc>2 ? atom_getintarg(2, argc, argv) : 1;
        fluid_synth_noteon(x->x_synth, chan - 1, key, vel);
    }
}

static void fluid_ctl(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!x->smmf_mode || x->x_synth == NULL)
        return;
    if(argc == 2 || argc == 3){
        int val = atom_getintarg(0, argc, argv);
        int ctrl = atom_getintarg(1, argc, argv);
        int chan = argc>2 ? atom_getintarg(2, argc, argv) : 1;
        fluid_synth_cc(x->x_synth, chan - 1, ctrl, val);
    }
}

static void fluid_pgm(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!x->smmf_mode || x->x_synth == NULL)
        return;
    if(argc == 1 || argc == 2){
        int prog = atom_getintarg(0, argc, argv);
        int chan = argc>1 ? atom_getintarg(1, argc, argv) : 1;
        fluid_synth_program_change(x->x_synth, chan-1, prog-1);
    }
}

static void fluid_polytouch(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!x->smmf_mode || x->x_synth == NULL)
        return;
    if(argc == 2 || argc == 3){
        int val = atom_getintarg(0, argc, argv);
        int key = atom_getintarg(1, argc, argv);
        int chan = argc>2 ? atom_getintarg(2, argc, argv) : 1;
        fluid_synth_key_pressure(x->x_synth, chan-1, key, val);
    }
}

static void fluid_touch(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!x->smmf_mode || x->x_synth == NULL)
        return;
    if(argc == 1 || argc == 2){
        int val = atom_getintarg(0, argc, argv);
        int chan = argc>1 ? atom_getintarg(1, argc, argv) : 1;
        fluid_synth_channel_pressure(x->x_synth, chan-1, val);
    }
}

static void fluid_bend(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(!x->smmf_mode){
        fluid_legacy_pitch_bend(x, s, argc, argv);
        return;
    }
    if(argc == 1 || argc == 2){
        int val = atom_getintarg(0, argc, argv);
        int chan = argc>1 ? atom_getintarg(1, argc, argv) : 1;
        fluid_synth_pitch_bend(x->x_synth, chan-1, val);
    }
}

// Maximum size of sysex data (excluding the f0 and f7 bytes) that we can
// handle. The size below should be plenty to handle any kind of MTS message,
// which at the time of this writing is the only kind of sysex message
// recognized by fluidsynth.
#define MAXSYSEXSIZE 1024

static void fluid_sysex(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!x->smmf_mode || x->x_synth == NULL)
        return;
    if(argc > 0){
        char buf[MAXSYSEXSIZE];
        int len = 0;
        while(len < MAXSYSEXSIZE && len < argc){
            buf[len] = atom_getintarg(len, argc, argv);
            len++;
        }
        // TODO: In order to handle bulk dump requests in the future, we will
        // have to pick up fluidsynth's response here and output that to a
        // control outlet (which doesn't exist at present).
        fluid_synth_sysex(x->x_synth, buf, len, NULL, NULL, NULL, 0);
    }
}

static void fluid_load(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(x->x_synth == NULL){
        pd_error(x, "[fluid~]: no fluidsynth");
        return;
    }
    if(argc >= 1 && argv->a_type == A_SYMBOL){
        const char* filename = atom_getsymbolarg(0, argc, argv)->s_name;
        const char* ext = strrchr(filename, '.');
        char realdir[MAXPDSTRING], *realname = NULL;
        int fd;
        if(ext && !strchr(ext, '/')){ // extension already supplied, no default extension
            post("extension supplied");
            ext = "";
            fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
            if(fd < 0){
              pd_error(x, "[fluid~]: can't find soundfont %s", filename);
              return;
            }
        }
        else{
            ext = ".sf2"; // let's try sf2
            fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
            if(fd < 0){ // failed
                ext = ".sf3"; // let's try sf3 then
                fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
                if(fd < 0){ // also failed
                    pd_error(x, "[fluid~]: can't find soundfont %s", filename);
                   return;
                }
            }
        }
        // Save the current working directory.
        char buf[MAXPDSTRING], *cwd = getcwd(buf, MAXPDSTRING);
        sys_close(fd);
        chdir(realdir);
        if(fluid_synth_sfload(x->x_synth, realname, 0) >= 0){
            post("[fluid~]: loaded soundfont %s", realname);
            fluid_synth_program_reset(x->x_synth);
        }
        // Restore the working directory.
        cwd && chdir(cwd);
    }
}

static void fluid_init(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    post("-- initializing [fluid~] for Pd --");
    if(x->x_synth)
        delete_fluid_synth(x->x_synth);
    if(x->x_settings)
        delete_fluid_settings(x->x_settings);
    float sr = sys_getsr();
    x->x_settings = new_fluid_settings();
    if(x->x_settings == NULL){
        pd_error(x, "[fluid~]: couldn't create synth settings\n");
        return;
    }
    else{ // load settings:
        fluid_settings_setnum(x->x_settings, "synth.midi-channels", 16);
        fluid_settings_setnum(x->x_settings, "synth.polyphony", 256);
        fluid_settings_setnum(x->x_settings, "synth.gain", 0.600000);
        fluid_settings_setnum(x->x_settings, "synth.sample-rate", sr != 0 ? sr : 44100.000000);
        fluid_settings_setstr(x->x_settings, "synth.chorus.active", "no");
        fluid_settings_setstr(x->x_settings, "synth.reverb.active", "no");
        fluid_settings_setstr(x->x_settings, "synth.ladspa.active", "no");
        x->x_synth = new_fluid_synth(x->x_settings); // Create fluidsynth instance:
        if(x->x_synth == NULL){
            pd_error(x, "[fluid~]: couldn't create synth");
            return;
        }
        if(atom_getsymbolarg(0, argc, argv) == gensym("-smmf")){ // check for SMMF mode
            x->smmf_mode = 1;
            argc--;
            argv++;
        }
        fluid_load(x, gensym("load"), argc, argv); // try to load argument as soundfont
        if(x->x_synth)  // Done
            post("-- [fluid~] loaded %s--", x->smmf_mode?" (with SMMF mode)":"");
        post("----------------------------------------------------");
    }
}

static void *fluid_tilde_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_fluid_tilde *x = (t_fluid_tilde *)pd_new(fluid_tilde_class);
    x->x_synth = NULL;
    x->x_settings = NULL;
    x->x_out_left = outlet_new(&x->x_obj, &s_signal);
    x->x_out_right = outlet_new(&x->x_obj, &s_signal);
    x->smmf_mode = 0;
    x->x_canvas = canvas_getcurrent();
    fluid_init(x, gensym("init"), argc, argv);
    return(void *)x;
}
 
void fluid_tilde_setup(void){
    fluid_tilde_class = class_new(gensym("fluid~"), (t_newmethod)fluid_tilde_new,
        (t_method)fluid_tilde_free, sizeof(t_fluid_tilde), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_init, gensym("init"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_load, gensym("load"), A_GIMME, 0);
    // "legacy" methods are enabled by default. NOTE: When in SMMF mode, the "note" and
    // "bend" messages invoke corresponding SMMF methods, while all other legacy methods
    // still work (in particular, the "note" and "bend" shortcuts are still available).
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_program_change, gensym("prog"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_control_change, gensym("control"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_bank, gensym("bank"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_gen, gensym("gen"), A_GIMME, 0);
    // alias shortcuts:
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_note, gensym("n"), A_GIMME, 0);
    class_addlist(fluid_tilde_class, (t_method)fluid_legacy_note);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_program_change, gensym("p"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_control_change, gensym("c"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_control_change, gensym("cc"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_pitch_bend, gensym("b"), A_GIMME, 0);
    // SMMF methods (new interface methods for MIDI, enabled with -smmf)
    class_addmethod(fluid_tilde_class, (t_method)fluid_note, gensym("note"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_ctl, gensym("ctl"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_pgm, gensym("pgm"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_polytouch, gensym("polytouch"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_touch, gensym("touch"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_bend, gensym("bend"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_sysex, gensym("sysex"), A_GIMME, 0);
    // Simulate Flext's help message
    class_addmethod(fluid_tilde_class, (t_method)fluid_help, gensym("help"), 0);
    post("----------------------------------------------------");
    post("[fluid~] 2.0 for Pd, using fluidsynth version: %s", FLUIDSYNTH_VERSION);
    post("----------------------------------------------------");
}
