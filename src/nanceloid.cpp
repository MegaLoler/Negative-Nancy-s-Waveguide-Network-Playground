#include <iostream>
#include <cmath>
#include <nanceloid.h>

double Nanceloid::get_frequency (double note) {
    return 440.0 * pow (2.0, (note - 69) / 12);
}

void Nanceloid::~Nanceloid (Voice *voice) {
    // NOTE: is this necessary?
    if (waveguide != nullptr)
        delete waveguide;
}

void Nanceloid::debug () {
    Parameters &p = parameters;

    cout << "[DEBUG PARAMETERS] " << endl;
    cout << "lungs              " << p.lungs << endl;
    cout << "glottal_tension    " << p.glottal_tension << endl;
    cout << "laryngeal_height   " << p.laryngeal_height << endl;
    cout << "lips_roundedness   " << p.lips_roundedness << endl;
    cout << "jaw_height         " << p.jaw_height << endl;
    cout << "tongue_frontness   " << p.tongue_frontness << endl;
    cout << "tongue_height      " << p.tongue_height << endl;
    cout << "tongue_flatness    " << p.tongue_flatness << endl;
    cout << "velic_closure      " << p.velic_closure << endl;
    cout << "acoustic_damping   " << p.acoustic_damping << endl;
    cout << "physical_damping   " << p.physical_damping << endl;
    cout << "enunciation        " << p.enunciation << endl;
    cout << "portamento         " << p.portamento << endl;
    cout << "frication          " << p.frication << endl;
    cout << "surface_tension    " << p.surface_tension << endl;
    cout << "tract_length       " << p.tract_length << endl;
    cout << "ambient_admittance " << p.ambient_admittance << endl;
    cout << "vibrato_rate       " << p.vibrato_rate << endl;
    cout << "vibrato_depth      " << p.vibrato_depth << endl;
    cout << "frequency          " << p.frequency << endl;
    cout << "volume             " << p.volume << endl;;
    cout << endl;
}

void Nanceloid::set_rate (int rate) {
    if (rate != this.rate) {
        this.rate = rate;
        init ();
    }
}

double Nanceloid::run () {

    // musical dynamics
    double vibrato = sin (vibrato_phase * 2 * M_PI) * parameters.vibrato_depth;
    vibrato_phase += parameters.vibrato_rate / rate;

    double frequency = get_frequency (note.note + note.detune + vibrato);
    parameters.frequency += (frequency - parameters.frequency) * parameters.portamento * portamento;

    parameters.lungs += (note.velocity - parameters.lungs) * parameters.portamento * portamento;

    // air pressure from lungs
    double opening = fmax (-parameters.glottal_tension, 0);
    waveguide->put (0, 0, parameters.lungs * opening);

    // vocal cord vibration
    waveguide->put (0, 0, source.run (this));

    // handle articulatory dynamics
    reshape ();

    // simulate acoustics
    waveguide->run ();

    // return the drain output
    return waveguide->collect_drain_right () * parameters.volume;
}

void Nanceloid::approach_admittance (Segment &segment, double target) {
    double current = segment.get_admittance ();
    double delta = (target - current) * parameters.enunciation * enunciation;
    double new_admittance = current + delta;
    segment.set_admittance (new_admittance);
}

void Nanceloid::reshape () {

    // shape the lips
    double lips_admittance = (1 - parameters.lips_roundedness) / neutral_impedance;
    for (int i = lips_start; i < waveguide->get_length (); i++) {
        Segment &segment = waveguide->get_segment (i);
        approach_admittance (segment, lips_admittance);
    }

    // shape the tongue
    for (int i = tongue_start; i < lips_start; i++) {
        Segment &segment = waveguide->get_segment (i);

        // TEMP
        // TODO: enhance this lol
        double unit_pos = i / (double) ((lips_start - tongue_start) - 1);
        double phase = unit_pos - parameters.tongue_frontness;
        double value = cos (phase * M_PI / 2) * arameters.tongue_height;
        double unit_area = 1 - value;
        approach_admittance (segment, unit_area / neutral_impedance);
    }

    //// shape the velum TODO
    //double velum_admittance = (1 - voice->parameters.velic_closure) / NASAL_Z;
    //set_admittance (voice->nose[0], velum_admittance);
}

void Nanceloid::init () {

    Waveguide *old = waveguide;

    // calculate the physical length of a single segment (cm)
    double unit = speed_of_sound / rate;

    // generate enough segments to meet the desired length
    int length = ceil (parameters.tract_length / unit);

    // create the new waveguide
    waveguide = new Waveguide (length, parameters.acoustic_damping, parameters.frication,
            INFINITY, parameters.ambient_admittance);

    // calculate the positions of various landmarks along the tract
    // TODO: calculate proportions based on overal tract length
    larynx_start = floor (length * 0);
    tongue_start = floor (length * 0.2);
    lips_start   = floor (length * 0.9);

    // if there was a previous waveguide, then copy the old sound state to the new one
    // and of course delete the old one
    if (old != nullptr) {
        // TODO: copy sound data
        // maybe make a waveguide method that copies sound in from old waveguide
        // (and fills in gaps to avoid subtle clicks)
        delete old;
    }
}