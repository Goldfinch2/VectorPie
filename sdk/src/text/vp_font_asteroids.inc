// The Asteroids font: Atari's arcade lettering as a vector font, from the
// USB-DVG firmware (asteroids_font.cpp). One entry per ASCII code 0..127;
// each glyph is up to 8 points on an 8 x 12 grid (4 bits x, 4 bits y, y up
// from the baseline). AF_UP lifts the pen; AF_LAST ends the glyph early.
// Lowercase letters are drawn as capitals; < and > are triangles.
// Included by vp_text.cpp only.

#define AF_UP   0xFE
#define AF_LAST 0xFF
#define AP(x, y) ((((x) & 0xF) << 4) | (((y) & 0xF) << 0))

static const AsteroidsGlyph ASTEROIDS[128] = {
    { AF_LAST },                                                                            // NUL
    { AF_LAST },                                                                            // SOH 
    { AF_LAST },                                                                            // STX 
    { AF_LAST },                                                                            // ETX
    { AF_LAST },                                                                            // EOT
    { AF_LAST },                                                                            // ENQ
    { AF_LAST },                                                                            // ACK
    { AF_LAST },                                                                            // BEL
    { AF_LAST },                                                                            // BS
    { AF_LAST },                                                                            // TAB
    { AF_LAST },                                                                            // LF
    { AF_LAST },                                                                            // VT
    { AF_LAST },                                                                            // FF
    { AF_LAST },                                                                            // CR
    { AF_LAST },                                                                            // SO
    { AF_LAST },                                                                            // SI
    { AF_LAST },                                                                            // DLE
    { AF_LAST },                                                                            // DC1
    { AF_LAST },                                                                            // DC2
    { AF_LAST },                                                                            // DC3
    { AF_LAST },                                                                            // DC4
    { AF_LAST },                                                                            // NAK
    { AF_LAST },                                                                            // SYN
    { AF_LAST },                                                                            // ETB
    { AF_LAST },                                                                            // CAN
    { AF_LAST },                                                                            // EM
    { AF_LAST },                                                                            // SUB
    { AF_LAST },                                                                            // ESC
    { AF_LAST },                                                                            // FS
    { AF_LAST },                                                                            // GS
    { AF_LAST },                                                                            // RS
    { AF_LAST },                                                                            // US
    { AF_LAST },                                                                            // SPACE
    { AP(4,0), AP(3,2), AP(5,2), AP(4,0), AF_UP, AP(4,4), AP(4,12), AF_LAST },                  // ! 
    { AP(2,10), AP(2,6), AF_UP, AP(6,10), AP(6,6), AF_LAST },                                 // " 
    { AP(0,4), AP(8,4), AP(6,2), AP(6,10), AP(8,8), AP(0,8), AP(2,10), AP(2,2) },                     // # 
    { AP(6,2), AP(2,6), AP(6,10), AF_UP, AP(4,12), AP(4,0), AF_LAST },                         // $ 
    { AP(0,0), AP(8,12), AF_UP, AP(2,10), AP(2,8), AF_UP, AP(6,4), AP(6,2) },                   // % 
    { AP(8,0), AP(4,12), AP(8,8), AP(0,4), AP(4,0), AP(8,4), AF_LAST },                           // & 
    { AP(2,6), AP(6,10), AF_LAST },                                                           // '
    { AP(6,0), AP(2,4), AP(2,8), AP(6,12), AF_LAST },                                           // ( 
    { AP(2,0), AP(6,4), AP(6,8), AP(2,12), AF_LAST },                                           // ) 
    { AP(0,0), AP(4,12), AP(8,0), AP(0,8), AP(8,8), AP(0,0), AF_LAST },                           // * 
    { AP(1,6), AP(7,6), AF_UP, AP(4,9), AP(4,3), AF_LAST },                                   // + 
    { AP(2,0), AP(4,2), AF_LAST },                                                            // , 
    { AP(2,6), AP(6,6), AF_LAST },                                                            // - 
    { AP(3,0), AP(4,0), AF_LAST },                                                            // . 
    { AP(0,0), AP(8,12), AF_LAST },                                                           // / 
    { AP(0,0), AP(8,0), AP(8,12), AP(0,12), AP(0,0), AP(8,12), AF_LAST },                         // 0
    { AP(4,0), AP(4,12), AP(3,10), AF_LAST },                                                  // 1 
    { AP(0,12), AP(8,12), AP(8,7), AP(0,5), AP(0,0), AP(8,0), AF_LAST },                          // 2 
    { AP(0,12), AP(8,12), AP(8,0), AP(0,0), AF_UP, AP(0,6), AP(8,6), AF_LAST },                 // 3 
    { AP(0,12), AP(0,6), AP(8,6), AF_UP, AP(8,12), AP(8,0), AF_LAST },                         // 4 
    { AP(0,0), AP(8,0), AP(8,6), AP(0,7), AP(0,12), AP(8,12), AF_LAST },                          // 5 
    { AP(0,12), AP(0,0), AP(8,0), AP(8,5), AP(0,7), AF_LAST },                                   // 6 
    { AP(0,12), AP(8,12), AP(8,6), AP(4,0), AF_LAST },                                          // 7 
    { AP(0,0), AP(8,0), AP(8,12), AP(0,12), AP(0,0), AF_UP, AP(0,6), AP(8,6), },                   // 8 
    { AP(8,0), AP(8,12), AP(0,12), AP(0,7), AP(8,5), AF_LAST },                                  // 9 
    { AP(4,9), AP(4,7), AF_UP, AP(4,5), AP(4,3), AF_LAST },                                   // : 
    { AP(4,9), AP(4,7), AF_UP, AP(4,5), AP(1,2), AF_LAST },                                   // ; 
    { AP(6,0), AP(2,6), AP(6,12), AP(6, 0), AF_LAST },                                          // LEFT TRIANGLE
    { AP(1,4), AP(7,4), AF_UP, AP(1,8), AP(7,8), AF_LAST },                                   // = 
    { AP(2,0), AP(6,6), AP(2,12), AP(2,0), AF_LAST },                                           // RIGHT TRIANGLE 
    { AP(0,8), AP(4,12), AP(8,8), AP(4,4), AF_UP, AP(4,1), AP(4,0), AF_LAST },                  // ? 
    { AP(8,4), AP(4,0), AP(0,4), AP(0,8), AP(4,12), AP(8,8), AP(4,4), AP(3,6) },                      // @ 
    { AP(0,0), AP(0,8), AP(4,12), AP(8,8), AP(8,0), AF_UP, AP(0,4), AP(8,4) },                     // A 
    { AP(0,0), AP(0,12), AP(4,12), AP(8,10), AP(4,6), AP(8,2), AP(4,0), AP(0,0) },                    // B 
    { AP(8,0), AP(0,0), AP(0,12), AP(8,12), AF_LAST },                                          // C 
    { AP(0,0), AP(0,12), AP(4,12), AP(8,8), AP(8,4), AP(4,0), AP(0,0), AF_LAST },                  // D 
    { AP(8,0), AP(0,0), AP(0,12), AP(8,12), AF_UP, AP(0,6), AP(6,6), AF_LAST },                 // E 
    { AP(0,0), AP(0,12), AP(8,12), AF_UP, AP(0,6), AP(6,6), AF_LAST },                         // F 
    { AP(6,6), AP(8,4), AP(8,0), AP(0,0), AP(0,12), AP(8,12), AF_LAST },                          // G 
    { AP(0,0), AP(0,12), AF_UP, AP(0,6), AP(8,6), AF_UP, AP(8,12), AP(8,0) },                   // H 
    { AP(0,0), AP(8,0), AF_UP, AP(4,0), AP(4,12), AF_UP, AP(0,12), AP(8,12) },                  // I 
    { AP(0,4), AP(4,0), AP(8,0), AP(8,12), AF_LAST },                                           // J 
    { AP(0,0), AP(0,12), AF_UP, AP(8,12), AP(0,6), AP(6,0), AF_LAST },                         // K 
    { AP(8,0), AP(0,0), AP(0,12), AF_LAST },                                                   // L 
    { AP(0,0), AP(0,12), AP(4,8), AP(8,12), AP(8,0), AF_LAST },                                  // M 
    { AP(0,0), AP(0,12), AP(8,0), AP(8,12), AF_LAST },                                          // N 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,0), AP(0,0), AF_LAST },                                  // O 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,6), AP(0,5), AF_LAST },                                  // P 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,4), AP(0,0), AF_UP, AP(4,4), AP(8,0) },                    // Q 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,6), AP(0,5), AF_UP, AP(4,5), AP(8,0) },                    // R 
    { AP(0,2), AP(2,0), AP(8,0), AP(8,5), AP(0,7), AP(0,12), AP(6,12), AP(8,10) },                    // S 
    { AP(0,12), AP(8,12), AF_UP, AP(4,12), AP(4,0), AF_LAST },                                // T 
    { AP(0,12), AP(0,2), AP(4,0), AP(8,2), AP(8,12), AF_LAST },                                  // U 
    { AP(0,12), AP(4,0), AP(8,12), AF_LAST },                                                  // V 
    { AP(0,12), AP(2,0), AP(4,4), AP(6,0), AP(8,12), AF_LAST },                                  // W 
    { AP(0,0), AP(8,12), AF_UP, AP(0,12), AP(8,0), AF_LAST },                                 // X 
    { AP(0,12), AP(4,6), AP(8,12), AF_UP, AP(4,6), AP(4,0), AF_LAST },                         // Y 
    { AP(0,12), AP(8,12), AP(0,0), AP(8,0), AF_UP, AP(2,6), AP(6,6), AF_LAST },                 // Z 
    { AP(6,0), AP(2,0), AP(2,12), AP(6,12), AF_LAST },                                          // [ 
    { AP(0,12), AP(8,0), AF_LAST },                                                           // BACKSLASH
    { AP(2,0), AP(6,0), AP(6,12), AP(2,12), AF_LAST },                                          // ] 
    { AP(2,6), AP(4,12), AP(6,6), AF_LAST },                                                   // ^ 
    { AP(0,0), AP(8,0), AF_LAST },                                                            // _ 
    { AP(2,10), AP(6,6), AF_LAST },                                                           // ` 
    { AP(0,0), AP(0,8), AP(4,12), AP(8,8), AP(8,0), AF_UP, AP(0,4), AP(8,4) },                     // A 
    { AP(0,0), AP(0,12), AP(4,12), AP(8,10), AP(4,6), AP(8,2), AP(4,0), AP(0,0) },                    // B 
    { AP(8,0), AP(0,0), AP(0,12), AP(8,12), AF_LAST },                                          // C 
    { AP(0,0), AP(0,12), AP(4,12), AP(8,8), AP(8,4), AP(4,0), AP(0,0), AF_LAST },                  // D 
    { AP(8,0), AP(0,0), AP(0,12), AP(8,12), AF_UP, AP(0,6), AP(6,6), AF_LAST },                 // E 
    { AP(0,0), AP(0,12), AP(8,12), AF_UP, AP(0,6), AP(6,6), AF_LAST },                         // F 
    { AP(6,6), AP(8,4), AP(8,0), AP(0,0), AP(0,12), AP(8,12), AF_LAST },                          // G 
    { AP(0,0), AP(0,12), AF_UP, AP(0,6), AP(8,6), AF_UP, AP(8,12), AP(8,0) },                   // H 
    { AP(0,0), AP(8,0), AF_UP, AP(4,0), AP(4,12), AF_UP, AP(0,12), AP(8,12) },                  // I 
    { AP(0,4), AP(4,0), AP(8,0), AP(8,12), AF_LAST },                                           // J 
    { AP(0,0), AP(0,12), AF_UP, AP(8,12), AP(0,6), AP(6,0), AF_LAST },                         // K 
    { AP(8,0), AP(0,0), AP(0,12), AF_LAST },                                                   // L 
    { AP(0,0), AP(0,12), AP(4,8), AP(8,12), AP(8,0), AF_LAST },                                  // M 
    { AP(0,0), AP(0,12), AP(8,0), AP(8,12), AF_LAST },                                          // N 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,0), AP(0,0), AF_LAST },                                  // O 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,6), AP(0,5), AF_LAST },                                  // P 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,4), AP(0,0), AF_UP, AP(4,4), AP(8,0) },                    // Q 
    { AP(0,0), AP(0,12), AP(8,12), AP(8,6), AP(0,5), AF_UP, AP(4,5), AP(8,0) },                    // R 
    { AP(0,2), AP(2,0), AP(8,0), AP(8,5), AP(0,7), AP(0,12), AP(6,12), AP(8,10) },                    // S 
    { AP(0,12), AP(8,12), AF_UP, AP(4,12), AP(4,0), AF_LAST },                                // T 
    { AP(0,12), AP(0,2), AP(4,0), AP(8,2), AP(8,12), AF_LAST },                                  // U 
    { AP(0,12), AP(4,0), AP(8,12), AF_LAST },                                                  // V 
    { AP(0,12), AP(2,0), AP(4,4), AP(6,0), AP(8,12), AF_LAST },                                  // W 
    { AP(0,0), AP(8,12), AF_UP, AP(0,12), AP(8,0), AF_LAST },                                 // X 
    { AP(0,12), AP(4,6), AP(8,12), AF_UP, AP(4,6), AP(4,0), AF_LAST },                         // Y 
    { AP(0,12), AP(8,12), AP(0,0), AP(8,0), AF_UP, AP(2,6), AP(6,6), AF_LAST },                 // Z  
    { AP(6,0), AP(4,2), AP(4,10), AP(6,12), AF_UP, AP(2,6), AP(4,6), AF_LAST },                 // { 
    { AP(4,0), AP(4,5), AF_UP, AP(4,6), AP(4,12), AF_LAST },                                  // | 
    { AP(4,0), AP(6,2), AP(6,10), AP(4,12), AF_UP, AP(6,6), AP(8,6), AF_LAST },                 // } 
    { AP(0,4), AP(2,8), AP(6,4), AP(8,8), AF_LAST },                                            // ~ 
    { AF_LAST },                                                                            // DEL
};

#undef AP
