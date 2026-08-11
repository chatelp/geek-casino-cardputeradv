// Entrées abstraites : chaque main (appareil / sim) mappe ses touches
// vers cet enum. La logique et l'UI ne connaissent que Key.
#pragma once

namespace hal {

enum class Key {
    None,
    Left,    // appareil : ','
    Right,   // appareil : '/'
    Up,      // appareil : ';'
    Down,    // appareil : '.'
    Enter,
    Space,   // spin
    Escape,  // retour / menu
};

}  // namespace hal
