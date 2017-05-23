/*
 *	emu3bm.c
 *	Copyright (C) 2016 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   EMU3 Filesystem Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   EMU3 Filesystem Tool is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with emu3bm.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emu3bm.h"

extern int verbosity;

const char *RT_CONTROLS_SRC[] = {
	"Pitch Control",
	"Mod Control",
	"Pressure Control",
	"Pedal Control",
	"MIDI A Control",
	"MIDI B Control"
};

const int RT_CONTROLS_SRC_SIZE = sizeof(RT_CONTROLS_SRC) / sizeof(char *);

const char *RT_CONTROLS_DST[] = {
	"Off",
	"Pitch",
	"VCF Cutoff",
	"VCA Level",
	"LFO -> Pitch",
	"LFO -> Cutoff",
	"LFO -> VCA",
	"Pan",
	"Attack",
	"Crossfade",
	"VCF NoteOn Q"
};

const int RT_CONTROLS_DST_SIZE = sizeof(RT_CONTROLS_DST) / sizeof(char *);

const char *RT_CONTROLS_FS_SRC[] = {
	"Footswitch 1",
	"Footswitch 2",
};

const int RT_CONTROLS_FS_SRC_SIZE =
	sizeof(RT_CONTROLS_FS_SRC) / sizeof(char *);

const char *RT_CONTROLS_FS_DST[] = {
	"Off",
	"Sustain",
	"Cross-Switch",
	"Unused 1",
	"Unused 2",
	"Unused 3",
	"Unused A",
	"Unused B",
	"Preset Increment",
	"Preset Decrement"
};

const int RT_CONTROLS_FS_DST_SIZE =
	sizeof(RT_CONTROLS_FS_DST) / sizeof(char *);

const char *LFO_SHAPE[] = {
	"triangle",
	"sine",
	"sawtooth",
	"square"
};

const char *VCF_TYPE[] = {
	"2 Pole Lowpass",
	"4 Pole Lowpass",
	"6 Pole Lowpass",
	"2nd Ord Hipass",
	"4th Ord Hipass",
	"2nd O Bandpass",
	"4th O Bandpass",
	"Contrary BandP",
	"Swept EQ 1 oct",
	"Swept EQ 2->1",
	"Swept EQ 3->1",
	"Phaser 1",
	"Phaser 2",
	"Bat-Phaser",
	"Flanger Lite",
	"Vocal Ah-Ay-Ee",
	"Vocal Oo-Ah",
	"Bottom Feeder",
	"ESi/E3x Lopass",
	"Unknown"
};

const int VCF_TYPE_SIZE = sizeof(VCF_TYPE) / sizeof(char *);

char *
emu3_e3name_to_filename(const char *objname)
{
	int i, size;
	const char *index = &objname[NAME_SIZE - 1];
	char *fname;

	for (size = NAME_SIZE; size > 0; size--) {
		if (*index != ' ')
			break;
		index--;
	}
	fname = (char*)malloc(size + 1);
	strncpy(fname, objname, size);
	fname[size] = '\0';
	for (i = 0; i < size; i++)
		if (fname[i] == '/')
			fname[i] = '?';

	return fname;
}

char *
emu3_e3name_to_wav_filename(const char *e3name)
{
	char *fname = emu3_e3name_to_filename(e3name);
	char *wname = malloc(strlen(fname) + 5);

	strcpy(wname, fname);
	strcat(wname, SAMPLE_EXT);
	return wname;
}

char *
emu3_wav_filename_to_filename(const char *wav_file)
{
	char *filename = malloc(strlen(wav_file) + 1);

	strcpy(filename, wav_file);
	char *ext = strrchr(wav_file, '.');
	if (strcmp(ext, SAMPLE_EXT) == 0) {
		free(filename);
		int len_wo_ext = strlen(wav_file) - strlen(SAMPLE_EXT);
		filename = malloc(len_wo_ext + 1);
		strncpy(filename, wav_file, len_wo_ext);
		filename[len_wo_ext] = '\0';
	}
	return filename;
}

char *
emu3_str_to_e3name(const char *src)
{
	int len = strlen(src);
	char *e3name = malloc(len + 1);

	strcpy(e3name, src);
	char *c = e3name;
	for (int i = 0; i < len; i++, c++)
		if (!isalnum(*c) && *c != ' ' && *c != '#')
			*c = '?';
	return e3name;
}

void
emu3_cpystr(char *dst, const char *src)
{
	int len = strlen(src);

	memcpy(dst, src, NAME_SIZE);
	memset(&dst[len], ' ', NAME_SIZE - len);
}


int
emu3_get_sample_channels(struct emu3_sample *sample)
{
	int channels;

	switch (sample->format) {
	case STEREO_SAMPLE_1:
	case STEREO_SAMPLE_2:
		channels = 2;
		break;
	case MONO_SAMPLE_1:
	case MONO_SAMPLE_2:
	case MONO_SAMPLE_EMULATOR_3X_1:
	case MONO_SAMPLE_EMULATOR_3X_2:
	case MONO_SAMPLE_EMULATOR_3X_3:
	case MONO_SAMPLE_EMULATOR_3X_4:
	case MONO_SAMPLE_EMULATOR_3X_5:
	default:
		channels = 1;
		break;
	}
	return channels;
}

void
emu3_print_sample_info(struct emu3_sample *sample, sf_count_t nframes)
{
	for (int i = 0; i < SAMPLE_PARAMETERS; i++)
		log(2, "0x%08x ", sample->parameters[i]);
	log(2, "\n");
	log(1, "Channels: %d\n", emu3_get_sample_channels(sample));
	log(1, "Frames: %" PRId64 "\n", nframes);
	log(1, "Sample rate: %dHz\n", sample->sample_rate);
}

//Level: [0, 0x7f] -> [0, 100]
int emu3_get_percent_value(char value)
{
	return (int)(value * 100 / 127.0);
}

//Pan: [0, 0x80] -> [-100, +100]
int emu3_get_vca_pan(char vca_pan)
{
	return (int)((vca_pan - 0x40) * 1.5625);
}

void
emu3_print_zone_info(struct emu3_preset_zone *zone)
{
	log(1, "VCA level: %d\n", emu3_get_percent_value(zone->vca_level));
	log(1, "VCA pan: %d\n", emu3_get_vca_pan(zone->vca_pan));

	log(1, "VCA envelope: attack: %d, hold: %d, decay: %d, sustain: %d\%, release: %d.\n",
	    zone->vca_envelope.attack, zone->vca_envelope.hold,
	    zone->vca_envelope.decay, emu3_get_percent_value(zone->vca_envelope.sustain),
	    zone->vca_envelope.release);

	//Filter type might only work with ESI banks
	int vcf_type = zone->vcf_type_lfo_shape >> 3;
	if (vcf_type > VCF_TYPE_SIZE - 1)
		vcf_type = VCF_TYPE_SIZE - 1;
	log(1, "VCF type %s\n", vcf_type, VCF_TYPE[vcf_type]);

	//Cutoff: [0, 255] -> [26, 74040]
	int cutoff = zone->vcf_cutoff;
	log(1, "VCF cutoff: %d\n", cutoff);

	//Filter Q might only work with ESI banks
	//ESI Q: [0x80, 0xff] -> [0, 100]
	//Other formats: [0, 0x7f]
	int q = (zone->vcf_q - 0x80) * 100 / 127;

	log(1, "VCF Q: %d\n", q);

	log(1, "VCF envelope amount: %d\n", emu3_get_percent_value(zone->vcf_envelope_amount));

	log(1, "VCF envelope: attack: %d, hold: %d, decay: %d, sustain: %d\%, release: %d.\n",
	    zone->vcf_envelope.attack, zone->vcf_envelope.hold,
	    zone->vcf_envelope.decay, emu3_get_percent_value(zone->vcf_envelope.sustain),
	    zone->vcf_envelope.release);

	log(1, "Auxiliary envelope amount: %d\n", emu3_get_percent_value(zone->aux_envelope_amount));

	log(1, "Auxiliary envelope: attack: %d, hold: %d, decay: %d, sustain: %d\%, release: %d.\n",
	    zone->aux_envelope.attack, zone->aux_envelope.hold,
	    zone->aux_envelope.decay, emu3_get_percent_value(zone->aux_envelope.sustain),
	    zone->aux_envelope.release);

	log(1, "Velocity to Pitch: %d\n", zone->vel_to_pitch);
	log(1, "Velocity to VCA Level: %d\n", emu3_get_percent_value(zone->vel_to_vca_level));
	log(1, "Velocity to VCA Attack: %d\n", zone->vel_to_vca_attack);
	log(1, "Velocity to VCF Cutoff: %d\n", zone->vel_to_vcf_cutoff);
	log(1, "Velocity to VCF Q: %d\n", zone->vel_to_vcf_q);
	log(1, "Velocity to VCF Attack: %d\n", zone->vel_to_vcf_attack);
	log(1, "Velocity to Pan: %d\n", emu3_get_vca_pan(zone->vel_to_vca_pan));
	log(1, "Velocity to Sample Start: %d\n", zone->vel_to_sample_start);
	log(1, "Velocity to Auxiliary Env: %d\n", zone->vel_to_aux_env);

	log(1, "LFO shape: %s\n", LFO_SHAPE[zone->vcf_type_lfo_shape & 0x3]);
	log(1, "LFO->Pitch: %d\n", emu3_get_percent_value(zone->lfo_to_pitch));
	log(1, "LFO->Cutoff: %d\n", emu3_get_percent_value(zone->lfo_to_cutoff));
	log(1, "LFO->VCA: %d\n", emu3_get_percent_value(zone->lfo_to_vca));
	log(1, "LFO->Pan: %d\n", emu3_get_percent_value(zone->lfo_to_pan));
}

void
emu3_print_preset_info(struct emu3_preset *preset)
{
	for (int i = 0; i < RT_CONTROLS_SRC_SIZE; i++) {
		int dst = 0;
		for (int j = 0; j < RT_CONTROLS_SIZE; j++) {
			if (preset->rt_controls[j] == i + 1) {
				dst = j + 1;
				break;
			}
		}
		log(1, "Mapping: %s - %s\n", RT_CONTROLS_SRC[i], RT_CONTROLS_DST[dst]);
	}
	for (int i = 0; i < RT_CONTROLS_FS_SIZE; i++) {
		log(1, "Mapping: %s - %s\n",
		    RT_CONTROLS_FS_SRC[i],
		    RT_CONTROLS_FS_DST[preset->rt_controls[RT_CONTROLS_SIZE + i]]);
	}
	for (int i = 0; i < UNKNOWN_PARAMETERS_SIZE; i++)
		log(2, "Unknown parameter %d: %d\n", i, preset->unknown_parameters[i]);
	log(1, "Pitch Bend Range: %x\n", preset->pitch_bend_range)
}

void
emu3_set_preset_rt_control(struct emu3_preset *preset, int src, int dst)
{
	if (dst >= 0 && dst < RT_CONTROLS_DST_SIZE) {
		log(0, "Setting controller %s to %s...\n",
		    RT_CONTROLS_SRC[src], RT_CONTROLS_DST[dst]);
		if (dst >= 0) {
			for (int i = 0; i < RT_CONTROLS_SIZE; i++)
				if (preset->rt_controls[i] == src + 1)
					preset->rt_controls[i] = 0;
			if (dst > 0)
				preset->rt_controls[dst - 1] = src + 1;
		}
	}else
		fprintf(stderr, "Invalid destination %d for %s\n", dst,
			RT_CONTROLS_SRC[src]);
}

void
emu3_set_preset_rt_control_fs(struct emu3_preset *preset, int src, int dst)
{
	if (dst >= 0 && dst < RT_CONTROLS_FS_DST_SIZE) {
		log(0, "Setting controller %s to %s...\n",
		    RT_CONTROLS_FS_SRC[src], RT_CONTROLS_FS_DST[dst]);
		preset->rt_controls[src + RT_CONTROLS_FS_DST_SIZE] = dst;
	}else
		fprintf(stderr, "Invalid destination %d for %s\n", dst,
			RT_CONTROLS_FS_SRC[src]);
}

void
emu3_set_preset_zone_level(struct emu3_preset_zone *zone, int level)
{
	if (level < 0 || level > 100)
		fprintf(stderr, "Value %d for level not in range [0, 100]\n", level);
	else{
		log(0, "Setting level to %d...\n", level);
		zone->vca_level = (unsigned char)(level * 127 / 100);
	}
}

void
emu3_set_preset_zone_cutoff(struct emu3_preset_zone *zone, int cutoff)
{
	if (cutoff < 0 || cutoff > 255)
		fprintf(stderr, "Value for cutoff %d not in range [0, 255]\n", cutoff);
	else{
		log(0, "Setting cutoff to %d...\n", cutoff);
		zone->vcf_cutoff = (unsigned char)cutoff;
	}
}

void
emu3_set_preset_zone_q(struct emu3_preset_zone *zone, int q)
{
	if (q < 0 || q > 100)
		fprintf(stderr, "Value %d for Q not in range [0, 100]\n", q);
	else{
		log(0, "Setting Q to %d...\n", q);
		zone->vcf_q = (unsigned char)((q * 127 / 100) + 0x80);
	}
}

void
emu3_set_preset_zone_filter(struct emu3_preset_zone *zone, int filter)
{
	if (filter < 0 || filter > VCF_TYPE_SIZE - 2)
		fprintf(stderr, "Value %d for filter not in range [0, %d]\n", filter,
			VCF_TYPE_SIZE - 2);
	else{
		log(0, "Setting filter to %s...\n", VCF_TYPE[filter]);
		zone->vcf_type_lfo_shape =
			((unsigned char)filter) << 3 | zone->vcf_type_lfo_shape & 0x3;
	}
}

void
emu3_set_preset_rt_controls(struct emu3_preset *preset, char *rt_controls)
{
	char *token;
	char *endtoken;
	int i;
	int controller;

	log(0, "Setting realtime controls...\n");
	i = 0;
	while (i < RT_CONTROLS_SIZE && (token = strsep(&rt_controls, ",")) != NULL) {
		if (*token == '\0')
			fprintf(stderr, "Empty value\n");
		else{
			controller = strtol(token, &endtoken, 10);
			if (*endtoken == '\0') {
				if (i < RT_CONTROLS_SRC_SIZE)
					emu3_set_preset_rt_control(preset, i, controller);
				else if (i < RT_CONTROLS_SRC_SIZE + RT_CONTROLS_FS_SRC_SIZE) {
					emu3_set_preset_rt_control_fs(preset,
								      i - RT_CONTROLS_SRC_SIZE,
								      controller);
				}else
					fprintf(stderr, "Too many controls\n");
			}else
				fprintf(stderr, "'%s' not a number\n", token);
		}
		i++;
	}
}

void
emu3_set_preset_pbr(struct emu3_preset * preset, int pbr)
{
	preset->pitch_bend_range = pbr;
}

void
emu3_init_sample_descriptor(struct emu3_sample_descriptor *sd,
			    struct emu3_sample *sample, int frames)
{
	sd->sample = sample;

	sd->l_channel = sample->frames;
	if (sample->format == STEREO_SAMPLE_1)
		//We consider the 4 shorts padding that the left channel has
		sd->r_channel = sample->frames + frames + 4;
}

void
emu3_write_frame(struct emu3_sample_descriptor *sd, short int frame[])
{
	*sd->l_channel = frame[0];
	sd->l_channel++;
	if (sd->sample->format == STEREO_SAMPLE_1) {
		*sd->r_channel = frame[1];
		sd->r_channel++;
	}
}

//returns the sample size in bytes that the the sample takes in the bank
int
emu3_append_sample(char *path, struct emu3_sample *sample)
{
	SF_INFO input_info;
	SNDFILE *input;
	int size = 0;
	unsigned int data_size;
	short int frame[2];
	short int zero[] = { 0, 0 };
	int mono_size;
	const char *filename;
	struct emu3_sample_descriptor sd;

	input_info.format = 0;
	input = sf_open(path, SFM_READ, &input_info);
	//TODO: add more formats
	if ((input_info.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
		fprintf(stderr, "Sample not in a valid format. Skipping...\n");
	else if (input_info.channels > 2)
		fprintf(stderr, "Sample neither mono nor stereo. Skipping...\n");
	else{
		filename = basename(path);
		log(1, "Appending sample %s... (%" PRId64 " frames, %d channels)\n",
		    filename, input_info.frames, input_info.channels);
		//Sample header initialization
		char *name = emu3_wav_filename_to_filename(filename);
		char *e3name = emu3_str_to_e3name(name);
		emu3_cpystr(sample->name, e3name);
		free(name);
		free(e3name);

		data_size = sizeof(short int) * (input_info.frames + 4);
		mono_size = sizeof(struct emu3_sample) + data_size;
		size = mono_size + (input_info.channels == 1 ? 0 : data_size);
		sample->parameters[0] = 0;
		//Start of left channel
		sample->parameters[1] = sizeof(struct emu3_sample);
		//Start of right channel
		sample->parameters[2] = input_info.channels == 1 ? 0 : mono_size;
		//Last sample of left channel
		sample->parameters[3] = mono_size - 2;
		//Last sample of right channel
		sample->parameters[4] = input_info.channels == 1 ? 0 : size - 2;

		int loop_start = 4; //This is an arbitrary value
		//Example (mono and stereo): Loop start @ 9290 sample is stored as ((9290 + 2) * 2) + sizeof (struct emu3_sample)
		sample->parameters[5] =
			((loop_start + 2) * 2) + sizeof(struct emu3_sample);
		//Example
		//Mono: Loop start @ 9290 sample is stored as (9290 + 2) * 2
		//Stereo: Frames * 2 + parameters[5] + 8
		sample->parameters[6] =
			input_info.channels ==
			1 ? (loop_start + 2) * 2 : input_info.frames * 2 +
			sample->parameters[5] + 8;

		int loop_end = input_info.frames - 10; //This is an arbitrary value
		//Example (mono and stereo): Loop end @ 94008 sample is stored as ((94008 + 1) * 2) + sizeof (struct emu3_sample)
		sample->parameters[7] =
			((loop_end + 1) * 2) + sizeof(struct emu3_sample);
		//Example
		//Mono: Loop end @ 94008 sample is stored as ((94008 + 1) * 2)
		//Stereo: Frames * 2 + parameters[7] + 8
		sample->parameters[8] =
			input_info.channels ==
			1 ? (loop_end + 1) * 2 : input_info.frames * 2 +
			sample->parameters[7] + 8;

		sample->sample_rate = DEFAULT_SAMPLING_FREQ;

		sample->format =
			input_info.channels == 1 ? MONO_SAMPLE_1 : STEREO_SAMPLE_1;

		for (int i = 0; i < MORE_SAMPLE_PARAMETERS; i++)
			sample->more_parameters[i] = 0;

		emu3_init_sample_descriptor(&sd, sample, input_info.frames);

		//2 first frames set to 0
		emu3_write_frame(&sd, zero);
		emu3_write_frame(&sd, zero);

		for (int i = 0; i < input_info.frames; i++) {
			sf_readf_short(input, frame, 1);
			emu3_write_frame(&sd, frame);
		}

		//2 end frames set to 0
		emu3_write_frame(&sd, zero);
		emu3_write_frame(&sd, zero);

		log(1, "Appended %dB (0x%08x).\n", size, size);
	}
	sf_close(input);

	return size;
}

void
emu3_write_sample_file(struct emu3_sample *sample, sf_count_t nframes)
{
	SF_INFO output_info;
	SNDFILE *output;
	char *wav_file;
	short *l_channel, *r_channel;
	short frame[2];
	char *schannels;
	int channels = emu3_get_sample_channels(sample);
	int samplerate = sample->sample_rate;

	output_info.frames = nframes;
	output_info.samplerate = samplerate;
	output_info.channels = channels;
	output_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	wav_file = emu3_e3name_to_wav_filename(sample->name);
	schannels = channels == 1 ? "mono" : "stereo";
	log(0, "Extracting %s sample %s...\n", schannels, wav_file);

	output = sf_open(wav_file, SFM_WRITE, &output_info);
	l_channel = sample->frames + 2;
	if (channels == 2)
		r_channel = sample->frames + nframes + 6;
	for (int i = 0; i < nframes; i++) {
		frame[0] = *l_channel;
		l_channel++;
		if (channels == 2) {
			frame[1] = *r_channel;
			r_channel++;
		}
		if (!sf_writef_short(output, frame, 1)) {
			fprintf(stderr, "Error: %s\n", sf_strerror(output));
			break;
		}
	}
	sf_close(output);
}

unsigned int
emu3_get_preset_address(struct emu3_bank *bank, unsigned int offset)
{
	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0)
		return PRESET_START_EMU_3X + offset;
	else if (strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		return PRESET_START_EMU_THREE + offset - PRESET_OFFSET_EMU_THREE;
	else
		return 0;
}

unsigned int
emu3_get_sample_start_address(struct emu3_bank *bank)
{
	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0)
		//There is always a 0xee byte before the samples
		return PRESET_START_EMU_3X + 1;
	else if (strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		//There is always a 0x00 byte before the samples
		return PRESET_START_EMU_THREE + 1 - PRESET_OFFSET_EMU_THREE;
	else
		return 0;
}

unsigned int *
emu3_get_sample_addresses(struct emu3_bank *bank)
{
	char * raw = (char*)bank;

	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0)
		return (unsigned int*)&(raw[SAMPLE_ADDR_START_EMU_3X]);
	else if (strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		return (unsigned int*)&(raw[SAMPLE_ADDR_START_EMU_THREE]);
	else
		return NULL;
}

unsigned int *
emu3_get_preset_addresses(struct emu3_bank *bank)
{
	char * raw = (char*)bank;

	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0)
		return (unsigned int*)&raw[PRESET_SIZE_ADDR_START_EMU_3X];
	else if (strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		return (unsigned int*)&raw[PRESET_SIZE_ADDR_START_EMU_THREE];
	else
		return NULL;
}

int
emu3_check_bank_format(struct emu3_bank *bank)
{
	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		return 1;
	else
		return 0;
}

int
emu3_get_max_samples(struct emu3_bank *bank)
{
	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0)
		return MAX_SAMPLES_EMU_3X;
	else if (strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		return MAX_SAMPLES_EMU_THREE;
	else
		return 0;
}

int
emu3_get_max_presets(struct emu3_bank *bank)
{
	if (strncmp(EMULATOR_3X_DEF, bank->format, FORMAT_SIZE) == 0 ||
	    strncmp(ESI_32_V3_DEF, bank->format, FORMAT_SIZE) == 0)
		return MAX_PRESETS_EMU_3X;
	else if (strncmp(EMULATOR_THREE_DEF, bank->format, FORMAT_SIZE) == 0)
		return MAX_PRESETS_EMU_THREE;

	else
		return 0;
}

void emu3_close_file(struct emu3_file * file)
{
	free(file->raw);
	free(file);
}

struct emu3_file * emu3_open_file(const char* filename)
{
	struct emu3_file * file;
	FILE *fd = fopen(filename, "r");

	if (fd) {
		file = (struct emu3_file*)malloc(sizeof(struct emu3_file));

		file->filename = filename;
		file->raw = (char*)malloc(MEM_SIZE);
		file->fsize = fread(file->raw, 1, MEM_SIZE, fd);
		fclose(fd);

		if (emu3_check_bank_format(file->bank)) {
			log(0, "Bank name: %.*s\n", NAME_SIZE, file->bank->name);
			log(1, "Bank fsize: %zuB\n", file->fsize);
			log(1, "Bank format: %s\n", file->bank->format);

			log(2, "Geometry:\n");
			log(2, "Objects: %d\n", file->bank->objects + 1);
			log(2, "Next: 0x%08x\n", file->bank->next);

			for (int i = 0; i < BANK_PARAMETERS; i++)
				log(2, "Parameter %2d: 0x%08x (%d)\n", i, file->bank->parameters[i],
				    file->bank->parameters[i]);

			if (file->bank->parameters[1] + file->bank->parameters[2] != file->bank->parameters[4])
				log(2, "Kind of checksum error.\n");

			if (strncmp(file->bank->name, file->bank->name_copy, NAME_SIZE))
				log(2, "Bank name is different than previously found.\n");

			log(2, "More geometry:\n");
			for (int i = 0; i < MORE_BANK_PARAMETERS; i++)
				log(2, "Parameter %d: 0x%08x (%d)\n", i, file->bank->more_parameters[i],
				    file->bank->more_parameters[i]);

			log(2, "Current preset: %d\n", file->bank->more_parameters[0]);
			log(2, "Current sample: %d\n", file->bank->more_parameters[1]);

			return file;
		}else {
			fprintf(stderr, "Bank format not supported.\n");
			emu3_close_file(file);
			return NULL;
		}
	} else {
		fprintf(stderr, "Error: file %s could not be opened.\n", filename);
		return NULL;
	}
}

int
emu3_process_bank(struct emu3_file * file, int xflg, char *rt_controls, int level, int cutoff, int q, int filter, int pbr)
{
	int size, channels;
	struct emu3_bank *bank;
	unsigned int *addresses;
	unsigned int address;
	struct emu3_preset *preset;
	struct emu3_preset_zone *zones;
	unsigned int sample_start_addr;
	unsigned int next_sample_addr;
	struct emu3_sample *sample;
	int max_samples;

	if (!file)
		return EXIT_FAILURE;

	addresses = emu3_get_preset_addresses(file->bank);
	for (int i = 0; i < emu3_get_max_presets(file->bank); i++) {
		if (addresses[0] != addresses[1]) {
			address = emu3_get_preset_address(file->bank, addresses[0]);
			preset = (struct emu3_preset *)&file->raw[address];
			log(0, "Preset %3d, %.*s", i, NAME_SIZE, preset->name);
			log(1, " @ 0x%08x", address);
			log(0, "\n");
			if (rt_controls)
				emu3_set_preset_rt_controls(preset, rt_controls);

			if (pbr != -1)
				emu3_set_preset_pbr(preset, pbr);

			emu3_print_preset_info(preset);

			zones = (struct emu3_preset_zone *)
				&file->raw[address + sizeof(struct emu3_preset) + preset->nzones * 4];
			log(1, "Zones: %d\n", preset->nzones);
			for (int j = 0; j < preset->nzones; j++) {
				log(1, "Zone %d\n", j);
				if (level != -1)
					emu3_set_preset_zone_level(&zones[j], level);
				if (cutoff != -1)
					emu3_set_preset_zone_cutoff(&zones[j], cutoff);
				if (q != -1)
					emu3_set_preset_zone_q(&zones[j], q);
				if (filter != -1)
					emu3_set_preset_zone_filter(&zones[j], filter);

				emu3_print_zone_info(&zones[j]);
			}
		}
		addresses++;
	}

	sample_start_addr = emu3_get_sample_start_address(file->bank) + addresses[0];
	log(1, "Sample start: 0x%08x\n", sample_start_addr);

	max_samples = emu3_get_max_samples(file->bank);
	addresses = emu3_get_sample_addresses(file->bank);
	log(1, "Start with offset: 0x%08x\n", addresses[0]);
	log(1, "Next with offset: 0x%08x\n", addresses[max_samples]);
	next_sample_addr =
		sample_start_addr + addresses[max_samples] - SAMPLE_OFFSET;
	log(1, "Next sample: 0x%08x\n", next_sample_addr);

	for (int i = 0; i < max_samples; i++) {
		if (addresses[i] == 0)
			break;
		address = sample_start_addr + addresses[i] - SAMPLE_OFFSET;
		if (addresses[i + 1] == 0)
			size = next_sample_addr - address;
		else
			size = addresses[i + 1] - addresses[i];
		sample = (struct emu3_sample *)&file->raw[address];
		channels = emu3_get_sample_channels(sample);
		//We divide between the bytes per frame (number of channels * 2 bytes)
		//The 16 or 8 bytes are the 4 or 8 short int used for padding.
		sf_count_t nframes =
			(size - sizeof(struct emu3_sample) -
			 (8 * channels)) / (2 * channels);
		log(0, "Sample %3d, %.*s", i + 1, NAME_SIZE, sample->name);
		log(1, " @ 0x%08x (size %dB, frames %" PRId64 ")", address, size, nframes);
		log(0, "\n");
		emu3_print_sample_info(sample, nframes);

		if (xflg)
			emu3_write_sample_file(sample, nframes);
	}

	return EXIT_SUCCESS;
}

int emu3_add_sample(struct emu3_file *file, char *sample_filename)
{
	int i;
	int max_samples = emu3_get_max_samples(file->bank);
	unsigned int * paddresses = emu3_get_preset_addresses(file->bank);
	unsigned int offset = paddresses[emu3_get_max_presets(file->bank)];
	unsigned int sample_start_addr = emu3_get_sample_start_address(file->bank) + offset;
	unsigned int * addresses = emu3_get_sample_addresses(file->bank);
	unsigned int next_sample_addr = sample_start_addr + addresses[max_samples] - SAMPLE_OFFSET;
	struct emu3_sample * sample = (struct emu3_sample *)&file->raw[next_sample_addr];

	for (i = 0; i < max_samples; i++)
		if (addresses[i] == 0)
			break;

	if (i == max_samples) {
		fprintf(stderr, "No more samples could be added.\n");
		return EXIT_FAILURE;
	}

	printf("Adding sample %d...\n", i + 1); //Sample number is 1 based
	unsigned int size = emu3_append_sample(sample_filename, sample);

	if (size) {
		file->bank->objects++;
		file->bank->next = next_sample_addr + size - sample_start_addr;
		addresses[i] = addresses[max_samples];
		addresses[max_samples] = file->bank->next + SAMPLE_OFFSET;
		return EXIT_SUCCESS;
	}else {
		fprintf(stderr, "Error while adding sample.\n");
		return EXIT_FAILURE;
	}
}

void emu3_write_file(struct emu3_file * file)
{
	FILE *fd = fopen(file->filename, "w");
	unsigned int * addresses = emu3_get_preset_addresses(file->bank);
	unsigned int offset = addresses[emu3_get_max_presets(file->bank)];
	unsigned int sample_start_addr = emu3_get_sample_start_address(file->bank) + offset;
	size_t filesize = file->bank->next + sample_start_addr;

	if (fd) {
		fwrite(file->raw, 1, filesize, fd);
		fclose(fd);
	} else
		fprintf(stderr, "Error while opening file for writing.\n");
}

int
emu3_create_bank(const char *ifile)
{
	struct emu3_bank bank;
	char *name = emu3_str_to_e3name(ifile);
	char *fname = emu3_e3name_to_filename(name);
	char *path =
		malloc(strlen(DATADIR) + strlen(PACKAGE) + strlen(EMPTY_BANK) + 3);
	int ret = sprintf(path, "%s/%s/%s", DATADIR, PACKAGE, EMPTY_BANK);

	if (ret < 0) {
		fprintf(stderr, "Error while creating full path");
		return EXIT_FAILURE;
	}

	char buf[BUFSIZ];
	size_t size;

	FILE *src = fopen(path, "rb");
	if (!src) {
		fprintf(stderr, "Error while opening %s for input\n", path);
		return EXIT_FAILURE;
	}

	FILE *dst = fopen(fname, "w+b");
	if (!dst) {
		fprintf(stderr, "Error while opening %s for output\n", fname);
		return EXIT_FAILURE;
	}

	while (size = fread(buf, 1, BUFSIZ, src))
		fwrite(buf, 1, size, dst);

	fclose(src);

	rewind(dst);

	if (fread(&bank, sizeof(struct emu3_bank), 1, dst)) {
		emu3_cpystr(bank.name, name);
		emu3_cpystr(bank.name_copy, name);
		rewind(dst);
		fwrite(&bank, sizeof(struct emu3_bank), 1, dst);
	}

	fclose(dst);

	free(name);
	free(fname);
	free(path);

	return EXIT_SUCCESS;
}
