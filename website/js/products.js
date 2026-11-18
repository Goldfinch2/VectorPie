/*
 * BeamForge Shop: the one config file.
 *
 * Everything the site shows about products, prices, PayPal and shipping is
 * here. Edit this file, commit, push: the site updates. See README.md.
 *
 * Anything marked TODO is a placeholder you must fill in before launch.
 * While a value is null the site shows a neutral "announced soon" text,
 * never a made-up number.
 */
window.SHOP = {
  site: {
    name: "BeamForge Shop",
    // Contact address (shown in the header, footer, policies).
    email: "vectorbeamforge@gmail.com",
    // TODO: your domain, once registered (used in README / canonical links only).
    domain: "beamforge.shop",
    currency: "USD",
    // Free reservation list (status "reserve"). Each reservation is emailed
    // to this FormSubmit address. The first submission sends an activation
    // email to the inbox; click it once, or reservations are not delivered.
    reserveFormAction: "https://formsubmit.co/vectorbeamforge@gmail.com",
    // Shown under the price on pre-order products. TODO: the real window.
    shipWindow: null, // e.g. "Q2 2027"
    // Shown in the footer and the seller box on the policies page.
    sellerCountry: "Canada"
  },

  paypal: {
    // "sdk": PayPal's current hosted buttons (needs clientId below).
    // "form": the classic hosted button form (needs no clientId).
    // The Business dashboard tells you which one your button is: the SDK
    // snippet has "hostedButtonId", the classic one has "hosted_button_id".
    mode: "sdk",
    // TODO: from developer.paypal.com > Apps & Credentials (Live, or Sandbox for testing).
    clientId: "BAAIwhMbp8ZaxAingBG_G_AVBlNYmNoJNiG6euhCjs8ynqpaHjZtydhkWrfO1cEECXKA2HIQOzxFt5B73Y",
    // true: buttons go to sandbox.paypal.com (test buyers, no real money).
    sandbox: false
  },

  // Flat shipping rate per order, per region, in USD. If direct PayPal sales
  // are turned back on, PayPal's postage settings must match these.
  shipping: [
    { region: "Canada", carrier: "Canada Post", rate: 20.00 },
    { region: "United States", carrier: "Chit Chats, delivered by USPS", rate: 20.00 },
    // TODO: real Europe rate once quoted.
    { region: "Europe", carrier: "Canada Post, delivered by the local post", rate: 50.00 }
  ],

  categories: [
    { id: "boards", name: "Vector Boards", href: "index.html?cat=boards" },
    { id: "adapters", name: "Cabinet Adapters", href: "adapters.html" }
  ],

  /*
   * status: "coming-soon" -> "reserve" -> "preorder" -> "available" -> "sold-out"
   *   "reserve": free reservation form, no payment (PayPal button kept for later)
   * preorderOpens: before this date a "preorder" product shows a countdown
   *   only; from it on, the PayPal button (if paypalButtonId is set).
   * paypalButtonId: null shows "Ordering isn't open yet" instead of a buy button.
   */
  products: [
    {
      id: "beamforge",
      sku: "BF-001",
      name: "BeamForge Vector Generator",
      shortName: "BeamForge",
      category: "boards",
      url: "beamforge.html",
      tagline: "About 20 million points a second: about 6.7× the USB-DVG, on the same protocol.",
      price: 275.00,        // USD, plus shipping (PayPal fee built in)
      deposit: null,        // TODO (optional): take a deposit instead of the full price
      status: "reserve",
      paypalButtonId: "ZE38MAWSJQWGU",
      images: [
        { src: "assets/beamforge-photo-rev1.jpg", alt: "BeamForge Rev 1.0 board", caption: "Preliminary photo: Rev 1.0 board. The production board may differ." }
      ],
      highlights: [
        "About 20M points/s, against the USB-DVG's 3M",
        "Same protocol as the USB-DVG",
        "Detected automatically by VectorPie",
        "Firmware updates from the VectorPie settings menu"
      ],
      specs: [
        ["Drawing speed", "About 20 million points per second"],
        ["Protocol", "Same as the USB-DVG"],
        ["Host connection", "USB-C, to the Pi's upper blue USB 3 port. Data cable included"],
        ["Raspberry Pi required", "Raspberry Pi 4 or Pi 5 with 1 GB memory or higher, running VectorPie. Pi 5 preferred for better performance"],
        ["Storage required", "SSD or SD card, 32 GB or higher. SSD strongly recommended"],
        ["Firmware updates", "From VectorPie: Settings → DVG → UPDATE..."],
        ["Monitor connection", "DE15 connector, same pinout as the USB-DVG. Monitor cable not included"],
        ["Monitor presets", "CRT TYPE presets such as WG6100, 19V2000 and Amplifone"],
        ["Adjustments", "XSIZE, YSIZE, XCTR and YCTR trimmers on the board. Speed, intensity and pincushion from VectorPie"],
        ["Power", "600 mA max"],
        ["PCB dimensions", "85 × 56 mm"],
        ["Weight", "105 g"]
      ],
      inTheBox: [
        "Assembled BeamForge board",
        "4 × M2.5 male-female standoffs, 20 mm, sized for a Raspberry Pi 5 with active cooler",
        "4 × M2.5 × 6 mm screws and 4 × M2.5 nuts",
        "GPIO stacking extender, 2 × 20 pins",
        "USB-A to USB-C data cable, about 15 cm (6 in), Pi to BeamForge"
      ]
    },
    {
      id: "asteroids-adapter",
      sku: "BF-AST",
      name: "Asteroids / Asteroids Deluxe Adapter",
      category: "adapters",
      url: "adapters.html#asteroids-adapter",
      tagline: "Bring VectorPie to your Asteroids or Asteroids Deluxe cabinet while keeping its original controls, start-button lights, vector monitor and audio amplifier.",
      price: 175.00,        // USD, plus shipping (expected; shown as "Expected price")
      status: "coming-soon",
      paypalButtonId: null,
      images: [
        { src: "assets/adapter-asteroids-render.jpg", alt: "3D render of the Asteroids / Asteroids Deluxe Adapter board", caption: "Preliminary 3D render. The production board may differ." },
        { src: "assets/adapter-asteroids.svg", alt: "Asteroids cabinet adapter illustration" }
      ],
      specs: [
        ["Cabinets", "Asteroids, Asteroids Deluxe"],
        ["Cabinet connection", "Original harness, on the adapter's card edge"],
        ["Vector input", "DE-15 from BeamForge or USB-DVG: X, Y and intensity"],
        ["Raspberry Pi power output", "5.1 V DC, up to 5 A (25.5 W)"],
        ["PACLED64 power output", "5.1 V, barrel lead, shared with the Pi supply"],
        ["Audio", "3.5 mm input to the original cabinet amplifier"]
      ],
      inTheBox: [
        "Adapter board",
        "4 × M2.5 male-female spacers, 10 mm, for the Pi / BeamForge stack",
        "Vector cable: DE-15 male to male, straight through, 30 cm, BeamForge to adapter",
        "USB power cable: USB-A to USB-C, about 15 cm (6 in), adapter to Pi",
        "Audio cable: 3.5 mm stereo, male to male"
      ]
    },
    {
      id: "space-duel-adapter",
      sku: "BF-SD",
      name: "Space Duel / Gravitar / Black Widow Adapter",
      category: "adapters",
      url: "adapters.html#space-duel-adapter",
      tagline: "Bring VectorPie to your Space Duel, Gravitar or Black Widow cabinet while keeping its original controls, lights, vector monitor and audio amplifier.",
      price: 175.00,        // USD, plus shipping (expected; shown as "Expected price")
      status: "coming-soon",
      paypalButtonId: null,
      images: [
        { src: "assets/adapter-space-duel-render.jpg", alt: "3D render of the Space Duel / Gravitar / Black Widow Adapter board", caption: "Preliminary 3D render. The production board may differ." },
        { src: "assets/adapter-space-duel.svg", alt: "Space Duel cabinet adapter illustration" }
      ],
      specs: [
        ["Cabinets", "Space Duel, Gravitar, Black Widow"],
        ["Cabinet connection", "Original P19 and P20 harness connectors, on the adapter's card edges"],
        ["Cabinet power input", "+22 V DC, from the edge connector"],
        ["Vector input", "DE-15 from BeamForge or USB-DVG: X, Y, red, green and blue"],
        ["Raspberry Pi power output", "5.1 V DC, up to 5 A (25.5 W)"],
        ["PACLED64 power output", "5.1 V, barrel lead, shared with the Pi supply"],
        ["Audio", "3.5 mm input to the original cabinet amplifier"]
      ],
      inTheBox: [
        "Adapter board",
        "4 × M2.5 male-female spacers, 10 mm, for the Pi / BeamForge stack",
        "Vector cable: DE-15 male to male, straight through, 30 cm, BeamForge to adapter",
        "USB power cable: USB-A to USB-C, about 15 cm (6 in), adapter to Pi",
        "Audio cable: 3.5 mm stereo, male to male"
      ]
    }
  ]
};
