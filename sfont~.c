// Soundfont player based on FluidSynth (https://www.fluidsynth.org/)
// by Porres and others

/*
LICENSE:
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "m_pd.h"
#include <fluidsynth.h>
#include <string.h>
#include <unistd.h>

#define MAXSYSEXSIZE 1024 // Size of sysex data list (excluding the F0 [240] and F7 [247] bytes)

static t_class *sfont_class;
 
static int printed;

typedef struct _sfont{
    t_object            x_obj;
    fluid_synth_t      *x_synth;
    fluid_settings_t   *x_settings;
    fluid_sfont_t      *x_sfont;
    fluid_preset_t     *x_preset;
    t_outlet           *x_out_left;
    t_outlet           *x_out_right;
    t_canvas           *x_canvas;
    t_symbol           *x_sfname;
    t_outlet           *x_info_out;
    int                 x_sysex;
    int                 x_verbosity;
    int                 x_count;
    int                 x_ready;
    int                 x_id;
    int                 x_bank;
    int                 x_pgm;
    t_atom              x_at[MAXSYSEXSIZE];
    unsigned char       x_type;
    unsigned char       x_data;
    unsigned char       x_channel;
}t_sfont;

t_int *sfont_perform(t_int *w){
    t_sfont *x = (t_sfont *)(w[1]);
    t_sample *left = (t_sample *)(w[2]);
    t_sample *right = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    fluid_synth_write_float(x->x_synth, n, left, 0, 1, right, 0, 1);
    return(w+5);
}

static void sfont_dsp(t_sfont *x, t_signal **sp){
    dsp_add(sfont_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void fluid_getversion(void){
    post("[sfont~] version 1.0-alpha (using fluidsynth %s)", FLUIDSYNTH_VERSION);
}

static void fluid_verbose(t_sfont *x, t_floatarg f){
    x->x_verbosity = f != 0;
}

static void fluid_panic(t_sfont *x){
    if(x->x_synth)
        fluid_synth_system_reset(x->x_synth);
}

static void fluid_transp(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac < 1 || ac > 2)
        return;
    float cents = atom_getfloatarg(0, ac, av);
    int ch = ac == 2 ? atom_getintarg(1, ac, av) : 0;
    fluid_synth_set_gen(x->x_synth, ch, GEN_FINETUNE, cents);
}

static void fluid_pan(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    float f1 = atom_getfloatarg(0, ac, av);
    int ch = atom_getintarg(1, ac, av);
    float pan = f1 < -1 ? -1 : f1 > 1 ? 1 : f1;
    fluid_synth_set_gen(x->x_synth, ch, GEN_PAN, pan*500);
}

static void fluid_note(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2 || ac == 3){
        int key = atom_getintarg(0, ac, av);
        int vel = atom_getintarg(1, ac, av);
        int chan = ac > 2 ? atom_getintarg(2, ac, av) : 1; // channel starts at zero???
        fluid_synth_noteon(x->x_synth, chan-1, key, vel);
    }
}

static void fluid_program_change(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int pgm = atom_getintarg(0, ac, av);
        x->x_pgm = pgm < 0 ? 0 : pgm > 127 ? 127 : pgm;
        int ch = ac > 1 ? atom_getintarg(1, ac, av) - 1: 0;
        int fail = fluid_synth_program_change(x->x_synth, ch, x->x_pgm);
        if(!fail){
            fluid_preset_t* preset = fluid_sfont_get_preset(x->x_sfont, x->x_bank, x->x_pgm);
            if(preset == NULL){
                if(x->x_verbosity)
                    post("[sfont~]: couldn't load progam %d in bank %d", x->x_pgm, x->x_bank);
            }
            else{
                const char* name = fluid_preset_get_name(preset);
                if(x->x_verbosity)
                    post("[sfont~]: loaded \"%s\" (bank %d, pgm %d) in channel %d",
                        name, x->x_bank, x->x_pgm, ch + 1);
                outlet_symbol(x->x_info_out, gensym(name));
            }
        }
        else 
            post("[sfont~]: couldn't load progam %d from bank %d into channel %d", x->x_pgm, x->x_bank, ch + 1);
    }
}

static void fluid_bank(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int bank = atom_getintarg(0, ac, av);
        if(bank < 0)
            bank = 0;
        int ch = ac > 1 ? atom_getintarg(1, ac, av) - 1 : 0;
        int fail = fluid_synth_bank_select(x->x_synth, ch, bank);
        if(!fail){
            x->x_bank = bank;
            fluid_preset_t* preset = fluid_sfont_get_preset(x->x_sfont, x->x_bank, x->x_pgm);
            if(preset == NULL){
                if(x->x_verbosity)
                    post("[sfont~]: couldn't load progam %d in bank %d", x->x_pgm, x->x_bank);
            }
            else{
                fluid_synth_program_reset(x->x_synth);
                const char* name = fluid_preset_get_name(preset);
                if(x->x_verbosity)
                    post("[sfont~]: loaded \"%s\" (bank %d, pgm %d) in channel %d",
                         name, x->x_bank, x->x_pgm, ch + 1);
                outlet_symbol(x->x_info_out, gensym(name));
            }
        }
        else
            post("[sfont~]: couldn't load bank %d", bank);
    }
}

/*static void fluid_preset(t_sfont *x, t_float bank, t_float pgm){

}*/

static void fluid_control_change(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2 || ac == 3){
        int ctrl = atom_getintarg(0, ac, av);
        int val = atom_getintarg(1, ac, av);
        int chan = ac > 2 ? atom_getintarg(2, ac, av) : 1;
        fluid_synth_cc(x->x_synth, chan-1, ctrl, val);
    }
}

static void fluid_pitch_bend(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int val = atom_getintarg(0, ac, av);
        int chan = ac > 1 ? atom_getintarg(1, ac, av) : 1;
        fluid_synth_pitch_bend(x->x_synth, chan-1, val);
    }
}

static void fluid_gen(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2){
        int param = atom_getintarg(1, ac, av);
        float value = atom_getfloatarg(2, ac, av);
        fluid_synth_set_gen(x->x_synth, 0, param, value);
    }
    else if(ac == 3){
        int chan = atom_getintarg(0, ac, av);
        float param = atom_getintarg(1, ac, av);
        float value = atom_getfloatarg(2, ac, av);
        fluid_synth_set_gen(x->x_synth, chan-1, param, value);
    }
}

/*static void fluid_remap(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(ac == 128){
        double pitches[128];
        for(int i = 0; i < 128; i++)
            pitches[i] = atom_getfloatarg(i, ac, av) * 100;
        int ch = 0;
        int tune_bank = 0;
        int tune_prog = 0;
        fluid_synth_activate_key_tuning(x->x_synth, tune_bank, tune_prog, "remap-tuning", pitches, 1);
        fluid_synth_activate_tuning(x->x_synth, ch, tune_bank, tune_prog, 1);
    }
    else
        post("wrong size");
}

static void fluid_octave_tuning(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(x->x_synth == NULL)
        return;
    if(ac == 12){
        double pitches[12];
        for(int i = 0; i < 12; i++)
            pitches[i] = atom_getfloatarg(i, ac, av);
        int tune_bank = 0;
        int tune_prog = 0;
        int ch = 0;
        fluid_synth_activate_octave_tuning(x->x_synth, tune_bank, tune_prog, "oct-tuning", pitches, 1);
        fluid_synth_activate_tuning(x->x_synth, ch, tune_bank, tune_prog, 1);
    }
    else
        post("wrong size");
}*/

static void fluid_aftertouch(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 || ac == 2){
        int val = atom_getintarg(0, ac, av);
        int chan = ac > 1 ? atom_getintarg(1, ac, av) : 1;
        fluid_synth_channel_pressure(x->x_synth, chan-1, val);
    }
}

static void fluid_polytouch(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 2 || ac == 3){
        int val = atom_getintarg(0, ac, av);
        int key = atom_getintarg(1, ac, av);
        int chan = ac > 2 ? atom_getintarg(2, ac, av) : 1;
        fluid_synth_key_pressure(x->x_synth, chan-1, key, val);
    }
}

static void fluid_sysex(t_sfont *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac > 0){
        char buf[MAXSYSEXSIZE];
        int len = 0;
        while(len < MAXSYSEXSIZE && len < ac){
            buf[len] = atom_getintarg(len, ac, av);
            len++;
        }
        // TODO: In order to handle bulk dump requests in the future, we will
        // have to pick up fluidsynth's response here and output that to a
        // control outlet (which doesn't exist at present).
        fluid_synth_sysex(x->x_synth, buf, len, NULL, NULL, NULL, 0);
    }
}

static void fluid_float(t_sfont *x, t_float f){ // raw midi input
    if(f >= 0 && f <= 255){
        unsigned char val = (unsigned char)f;
        if(val > 127){ // not a data type
            if(val == 0xF0){ // start of sysex
                x->x_sysex = 1;
                x->x_count = 0;
            }
            else if(val == 0xF7){ // end of sysex
                fluid_sysex(x, &s_list, x->x_count, x->x_at);
                x->x_sysex = x->x_count = 0;
            }
            else{
                x->x_type = val & 0xF0; // get type
                x->x_channel = (val & 0x0F) + 1; // get channel
                x->x_ready = (x->x_type == 0xC0 || x->x_type == 0xD0); // ready if program or touch
            }
        }
        else if(x->x_sysex){
            SETFLOAT(&x->x_at[x->x_count], (t_float)val);
            x->x_count++;
        }
        else{
            if(x->x_ready){
                switch(x->x_type){
                    case 0x80: // group 128-143 (NOTE OFF)
                    SETFLOAT(&x->x_at[0], (t_float)x->x_data); // key
                    SETFLOAT(&x->x_at[1], 0); // make it note on with velocity 0
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    fluid_note(x, &s_list, 3, x->x_at);
                    break;
                case 0x90: // group 144-159 (NOTE ON)
                    SETFLOAT(&x->x_at[0], (t_float)x->x_data); // key
                    SETFLOAT(&x->x_at[1], val); // velocity
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    fluid_note(x, &s_list, 3, x->x_at);
                    break;
                case 0xA0: // group 160-175 (POLYPHONIC AFTERTOUCH)
                    SETFLOAT(&x->x_at[0], val); // aftertouch pressure value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_data); // key
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    fluid_polytouch(x, &s_list, 3, x->x_at);
                    break;
                case 0xB0: // group 176-191 (CONTROL CHANGE)
                    SETFLOAT(&x->x_at[0], val); // control value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_data); // control number
                    SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
                    fluid_control_change(x, &s_list, 3, x->x_at);
                    break;
                case 0xC0: // group 192-207 (PROGRAM CHANGE)
                    SETFLOAT(&x->x_at[0], val); // control value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_channel);
                    fluid_program_change(x, &s_list, 2, x->x_at);
                    break;
                case 0xD0: // group 208-223 (AFTERTOUCH)
                    SETFLOAT(&x->x_at[0], val); // control value
                    SETFLOAT(&x->x_at[1], (t_float)x->x_channel);
                    fluid_aftertouch(x, &s_list, 2, x->x_at);
                    break;
                case 0xE0: // group 224-239 (PITCH BEND)
                    SETFLOAT(&x->x_at[0], (val << 7) + x->x_data);
                    SETFLOAT(&x->x_at[1], x->x_channel);
                    fluid_pitch_bend(x, &s_list, 2, x->x_at);
                    break;
                default:
                    break;
                }
                x->x_type = x->x_ready = 0; // clear
            }
            else{ // not ready, get data and make it ready
                x->x_data = val;
                x->x_ready = 1;
            }
        }
    }
    else
        x->x_type = x->x_ready = 0; // clear
}

static void fluid_info(t_sfont *x){
    if(x->x_sfname == NULL){
        post("[sfont~]: no soundfont loaded, nothing to print");
        return;
    }
    post("Loaded soundfont: %s", fluid_sfont_get_name(x->x_sfont));
    post("------------------- presets -------------------");
    int i = 1;
    fluid_preset_t* preset;
    fluid_sfont_iteration_start(x->x_sfont);
    while((preset = fluid_sfont_iteration_next(x->x_sfont))){
        int bank = fluid_preset_get_banknum(preset), pgm = fluid_preset_get_num(preset);
        const char* name = fluid_preset_get_name(preset);
        post("%03d - bank (%d) pgm (%d) name (%s)", i++, bank, pgm, name);
    }
/*    for(i = 0; i < 16; i++){
        post("i (%d)", i);
        if((preset = fluid_synth_get_channel_preset(x->x_synth, i)) != NULL){
           int bank = fluid_preset_get_banknum(preset), pgm = fluid_preset_get_num(preset);
           const char* name = fluid_preset_get_name(preset);
           post("channel %d: bank (%d) pgm (%d) name (%s)", i++, bank, pgm, name);
        }
    }*/ // didn't fucking work... motherfucker
    post("\n");
}

static void fluid_load(t_sfont *x, t_symbol *name){
    const char* filename = name->s_name;
    const char* ext = strrchr(filename, '.');
    char realdir[MAXPDSTRING], *realname = NULL;
    int fd;
    if(ext && !strchr(ext, '/')){ // extension already supplied, no default extension
        ext = "";
        fd = canvas_open(x->x_canvas, filename, ext, realdir, &realname, MAXPDSTRING, 0);
        if(fd < 0){
            pd_error(x, "[sfont~]: can't find soundfont %s", filename);
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
                pd_error(x, "[sfont~]: can't find soundfont %s", filename);
                return;
            }
        }
    }
    sys_close(fd); // ???
    chdir(realdir);
    int id = fluid_synth_sfload(x->x_synth, realname, 0);
    if(id >= 0){
        fluid_synth_program_reset(x->x_synth);
        x->x_sfont = fluid_synth_get_sfont_by_id(x->x_synth, id);
        x->x_sfname = name;
        if(x->x_verbosity)
            fluid_info(x);
        fluid_preset_t* preset = fluid_sfont_get_preset(x->x_sfont, x->x_bank = 0, x->x_pgm = 0);
        if(preset){
            const char* pname = fluid_preset_get_name(preset);
            outlet_symbol(x->x_info_out, gensym(pname));
        }
    }
    else
        post("[sfont~]: couldn't load %d", realname);
}

static void sfont_free(t_sfont *x){
    if(x->x_synth)
        delete_fluid_synth(x->x_synth);
    if(x->x_settings)
        delete_fluid_settings(x->x_settings);
}

static void *sfont_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_sfont *x = (t_sfont *)pd_new(sfont_class);
    x->x_synth = NULL;
    x->x_settings = NULL;
    x->x_sfname = NULL;
    x->x_canvas = canvas_getcurrent();
    x->x_sysex = x->x_ready = x->x_count = 0;
    x->x_data = x->x_channel = x->x_type = 0;
    x->x_out_left = outlet_new(&x->x_obj, &s_signal);
    x->x_out_right = outlet_new(&x->x_obj, &s_signal);
    x->x_info_out = outlet_new((t_object *)x, gensym("list"));
    x->x_settings = new_fluid_settings();
    if(x->x_settings == NULL){
        pd_error(x, "[sfont~]: bug couldn't create synth settings\n");
        return(NULL);
    }
    int ch = 16;
    int arg = 0;
    double g = 0.4;
    t_symbol *filename = NULL;
    while(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-v") && !arg){
                x->x_verbosity = 1;
                if(!printed){
                    fluid_getversion();
                    printed = 1;
                }
                ac--, av++;
            }
            else if(sym == gensym("-ch") && !arg){
                ac--, av++;
                if(ac && av->a_type == A_FLOAT){
                    ch = atom_getfloatarg(0, ac, av);
                    if(ch < 16)
                        ch = 16;
                    if(ch > 256)
                        ch = 256;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else if(sym == gensym("-g") && !arg){
                ac--, av++;
                if(ac && av->a_type == A_FLOAT){
                    g = (double)atom_getfloatarg(0, ac, av);
                    if(g < 0.1)
                        g = 0.1;
                    if(g > 1)
                        g = 1;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else if(!arg){
                arg = 1;
                filename = sym;
                ac--, av++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    fluid_settings_setint(x->x_settings, "synth.ladspa.active", 0);
    fluid_settings_setint(x->x_settings, "synth.midi-channels", ch);
//  fluid_settings_setint(x->x_settings, "synth.polyphony", 256);
    fluid_settings_setnum(x->x_settings, "synth.gain", g);
    fluid_settings_setnum(x->x_settings, "synth.sample-rate", sys_getsr());
    x->x_synth = new_fluid_synth(x->x_settings); // Create fluidsynth instance:
    if(x->x_synth == NULL){
        pd_error(x, "[sfont~]: bug couldn't create fluidsynth instance");
        return(NULL);
    }
    if(filename)
        fluid_load(x, filename);
    return(x);
errstate:
    pd_error(x, "[sfont~]: wrong args");
    return(NULL);
}
 
void sfont_tilde_setup(void){
    sfont_class = class_new(gensym("sfont~"), (t_newmethod)sfont_new,
        (t_method)sfont_free, sizeof(t_sfont), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)sfont_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(sfont_class, (t_method)fluid_float); // raw midi input
    class_addmethod(sfont_class, (t_method)fluid_load, gensym("load"), A_SYMBOL, 0);
    class_addmethod(sfont_class, (t_method)fluid_gen, gensym("gen"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_bank, gensym("bank"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_note, gensym("note"), A_GIMME, 0);
    class_addlist(sfont_class, (t_method)fluid_note); // list input is the same as "note"
    class_addmethod(sfont_class, (t_method)fluid_control_change, gensym("ctl"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_program_change, gensym("pgm"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_aftertouch, gensym("touch"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_polytouch, gensym("polytouch"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_pitch_bend, gensym("bend"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_sysex, gensym("sysex"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_panic, gensym("panic"), 0);
    class_addmethod(sfont_class, (t_method)fluid_transp, gensym("transp"), A_GIMME, 0);
    class_addmethod(sfont_class, (t_method)fluid_pan, gensym("pan"), A_GIMME, 0);
//    class_addmethod(sfont_class, (t_method)fluid_remap, gensym("remap"), A_GIMME, 0);
//    class_addmethod(sfont_class, (t_method)fluid_scale, gensym("scale"), A_GIMME, 0);
//    class_addmethod(sfont_class, (t_method)fluid_a4, gensym("a4"), A_FLOAT, 0);
    class_addmethod(sfont_class, (t_method)fluid_getversion, gensym("version"), 0);
    class_addmethod(sfont_class, (t_method)fluid_verbose, gensym("verbose"), A_FLOAT, 0);
    class_addmethod(sfont_class, (t_method)fluid_info, gensym("info"), 0);
}

// let's steal ideas from:
// https://github.com/uliss/pure-data/blob/ceammc/ceammc/ext/src/misc/fluid.cpp

// https://www.fluidsynth.org/api/

// http://www.synthfont.com/sfspec24.pdf
