/*
 * BeamForge Shop: shared header, sidebar, footer, product rendering, tabs,
 * countdown and PayPal buttons. All data comes from js/products.js.
 */
(function () {
  "use strict";

  var SHOP = window.SHOP;
  var site = SHOP.site;

  /* ---------- helpers ---------- */

  function esc(s) {
    return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c];
    });
  }
  function $(sel, root) { return (root || document).querySelector(sel); }
  function $all(sel, root) { return Array.prototype.slice.call((root || document).querySelectorAll(sel)); }
  function product(id) {
    for (var i = 0; i < SHOP.products.length; i++) if (SHOP.products[i].id === id) return SHOP.products[i];
    return null;
  }
  function params() { return new URLSearchParams(location.search); }

  // The current time. On a local server only, ?now=2026-12-02 fakes it (for testing).
  function now() {
    var local = /^(localhost|127\.0\.0\.1|\[::1\])$/.test(location.hostname);
    var fake = local && params().get("now");
    if (fake) {
      var d = parseDate(fake);
      if (!isNaN(d)) return d;
    }
    return new Date();
  }
  // "2026-11-18" is midnight local time on that day.
  function parseDate(s) {
    var m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(s);
    return m ? new Date(+m[1], +m[2] - 1, +m[3]) : new Date(s);
  }
  function fmtDate(s) {
    return parseDate(s).toLocaleDateString("en-US", { year: "numeric", month: "long", day: "numeric" });
  }
  function money(v) {
    if (v == null) return null;
    return "$" + Number(v).toFixed(2) + " " + site.currency;
  }
  function emailOk() { return site.email && !/^TODO/.test(site.email); }

  /*
   * What a product shows:
   *   coming-soon    greyed card, COMING SOON badge, Details link
   *   reserve        FREE RESERVATION badge, reservation form, no payment
   *   preorder-soon  preorder before preorderOpens: countdown
   *   preorder       PayPal button (or "not open yet" note without a button ID)
   *   available      PayPal button (or "not open yet" note without a button ID)
   *   sold-out       no button
   */
  function state(p) {
    var s = p.status;
    if (s === "preorder" && p.preorderOpens && now() < parseDate(p.preorderOpens)) s = "preorder-soon";
    var buyable = (s === "preorder" || s === "available") && !!p.paypalButtonId;
    return { status: s, buyable: buyable };
  }

  function badge(p) {
    var st = state(p).status;
    if (st === "coming-soon") return '<span class="badge badge-accent">COMING SOON</span>';
    if (st === "reserve") return '<span class="badge badge-accent">FREE RESERVATION</span>';
    if (st === "preorder-soon") return '<span class="badge badge-accent">PRE-ORDERS OPEN ' + esc(fmtDate(p.preorderOpens).toUpperCase()) + "</span>";
    if (st === "preorder") return '<span class="badge badge-accent">PRE-ORDER' + (site.shipWindow ? ": SHIPS " + esc(site.shipWindow.toUpperCase()) : "") + "</span>";
    if (st === "available") return '<span class="badge badge-ok">IN STOCK</span>';
    return '<span class="badge badge-muted">SOLD OUT</span>';
  }

  function priceHtml(p, big) {
    var cls = big ? "price price-big" : "price";
    if (p.price == null) return '<div class="' + cls + ' price-tba">Price announced soon</div>';
    return '<div class="' + cls + '">' + esc(money(p.price)) +
      (big ? ' <span class="plus-ship">plus <a href="policies.html#shipping">shipping</a></span>' : "") + "</div>";
  }

  /* ---------- header / nav / footer ---------- */

  function renderHeader() {
    var el = $("#site-header");
    if (!el) return;
    var q = params().get("q") || "";
    el.innerHTML =
      '<div class="topbar"><div class="wrap">' +
        '<span>Ships from ' + esc(site.sellerCountry) + ' to Canada, the US and Europe</span>' +
        '<span class="topbar-right">' + (emailOk() ? '<a href="mailto:' + esc(site.email) + '">' + esc(site.email) + "</a>" : '<a href="faq.html#contact">Contact</a>') + "</span>" +
      "</div></div>" +
      '<div class="masthead"><div class="wrap">' +
        '<button class="menu-toggle" type="button" aria-expanded="false" aria-controls="site-nav" aria-label="Menu"><span></span><span></span><span></span></button>' +
        '<a class="logo" href="index.html" aria-label="' + esc(site.name) + ' home"><img src="assets/beamforge-logo.svg" alt="BeamForge" width="268" height="40"></a>' +
        '<form class="search" action="index.html" role="search">' +
          '<input type="search" name="q" placeholder="Search products" aria-label="Search products" value="' + esc(q) + '">' +
          '<button type="submit">Search</button>' +
        "</form>" +
        '<a class="my-order" href="faq.html#my-order"><svg viewBox="0 0 24 24" width="22" height="22" aria-hidden="true"><path fill="currentColor" d="M7 18a2 2 0 1 0 0 4 2 2 0 0 0 0-4Zm10 0a2 2 0 1 0 0 4 2 2 0 0 0 0-4ZM5.2 4H2V2h4.8l.9 2H21l-3.4 8H8.4l-1 2H19v2H4.2l2.6-5.1L5.2 4Z"/></svg><span>My order</span></a>' +
      "</div></div>" +
      '<nav class="catbar" id="site-nav" aria-label="Main"><div class="wrap">' +
        navLink("index.html?cat=boards", "Vector Boards", "boards") +
        navLink("adapters.html", "Cabinet Adapters", "adapters") +
        navLink("faq.html#support", "Support", "support") +
        navLink("faq.html", "FAQ", "faq") +
        navLink("faq.html#contact", "Contact", "contact") +
      "</div></nav>";

    var t = $(".menu-toggle", el);
    t.addEventListener("click", function () {
      var open = document.body.classList.toggle("nav-open");
      t.setAttribute("aria-expanded", open ? "true" : "false");
    });
  }
  function navLink(href, text, key) {
    var cur = document.body.getAttribute("data-nav") === key ? ' aria-current="page"' : "";
    return '<a href="' + href + '"' + cur + ">" + esc(text) + "</a>";
  }

  function renderCrumbs() {
    var el = $("#crumbs");
    if (!el) return;
    var parts = (el.getAttribute("data-crumbs") || "").split(";").filter(Boolean);
    el.innerHTML = '<div class="wrap"><ol>' + parts.map(function (p, i) {
      var bits = p.split("|");
      var last = i === parts.length - 1;
      return "<li>" + (bits[1] && !last ? '<a href="' + esc(bits[1]) + '">' + esc(bits[0]) + "</a>" : '<span aria-current="page">' + esc(bits[0]) + "</span>") + "</li>";
    }).join("") + "</ol></div>";
  }

  function renderSidebar() {
    var el = $("#sidebar");
    if (!el) return;
    var cats = SHOP.categories.map(function (c) {
      var n = SHOP.products.filter(function (p) { return p.category === c.id; }).length;
      return '<li><a href="' + esc(c.href) + '">' + esc(c.name) + ' <span class="count">(' + n + ")</span></a></li>";
    }).join("");
    var soon = SHOP.products.filter(function (p) { return state(p).status === "coming-soon"; }).map(function (p) {
      return '<li><a href="' + esc(p.url) + '">' + esc(p.name) + '</a></li>';
    }).join("");
    el.innerHTML =
      '<div class="side-box"><h3>Categories</h3><ul class="side-list">' + cats + "</ul></div>" +
      (soon ? '<div class="side-box side-soon"><h3>Coming soon</h3><ul class="side-list">' + soon + "</ul></div>" : "") +
      '<div class="side-box side-help"><h3>Help</h3><ul class="side-list">' +
        '<li><a href="faq.html#ordering">Ordering</a></li>' +
        '<li><a href="policies.html#shipping">Shipping rates</a></li>' +
        '<li><a href="policies.html#refunds">Refunds</a></li>' +
      "</ul></div>";
  }

  function renderFooter() {
    var el = $("#site-footer");
    if (!el) return;
    el.innerHTML =
      '<div class="wrap footer-cols">' +
        '<div><h4>Shop</h4><ul>' +
          '<li><a href="beamforge.html">BeamForge</a></li>' +
          '<li><a href="adapters.html">Cabinet Adapters</a></li>' +
        "</ul></div>" +
        '<div><h4>Help</h4><ul>' +
          '<li><a href="faq.html">FAQ</a></li>' +
          '<li><a href="faq.html#support">Support</a></li>' +
          '<li><a href="faq.html#contact">Contact</a></li>' +
        "</ul></div>" +
        '<div><h4>Policies</h4><ul>' +
          '<li><a href="policies.html#shipping">Shipping</a></li>' +
          '<li><a href="policies.html#refunds">Refunds &amp; returns</a></li>' +
          '<li><a href="policies.html#privacy">Privacy</a></li>' +
        "</ul></div>" +
        '<div class="footer-pay"><h4>We accept</h4><span class="paypal-mark" aria-label="PayPal"><b>Pay</b><i>Pal</i></span>' +
          '<p class="small">Payments are processed by PayPal. We never see your card details.</p></div>' +
      "</div>" +
      '<div class="footer-legal"><div class="wrap">' +
        "<p>© " + new Date().getFullYear() + " " + esc(site.name) + ". All prices in " + esc(site.currency) + ".</p>" +
        '<p class="small">Asteroids, Asteroids Deluxe, Space Duel, Gravitar and Black Widow are trademarks of their respective owners. They are used here only to describe compatibility. ' +
        "BeamForge is not affiliated with, sponsored or endorsed by Atari or any other cabinet maker.</p>" +
      "</div></div>";
  }

  /* ---------- product cards / grids ---------- */

  function card(p) {
    var st = state(p);
    var muted = st.status === "coming-soon" || st.status === "sold-out";
    var action;
    if (st.status === "coming-soon") action = '<a class="btn btn-outline" href="' + esc(p.url) + '">Details</a>';
    else if (st.status === "sold-out") action = '<span class="btn btn-disabled" aria-disabled="true">Sold out</span>';
    else if (st.status === "reserve") action = '<a class="btn" href="reserve.html">Reserve ' + esc(p.shortName || p.name) + '</a>';
    else action = '<a class="btn" href="' + esc(p.url) + '">' + (st.buyable ? (st.status === "preorder" ? "Pre-order" : "Buy now") : "Details") + "</a>";
    return '<article class="card' + (muted ? " card-muted" : "") + '" data-product="' + esc(p.id) + '" data-status="' + st.status + '">' +
      '<a class="card-img" href="' + esc(p.url) + '"><img src="' + esc(p.images[0].src) + '" alt="' + esc(p.images[0].alt) + '" loading="lazy"></a>' +
      '<div class="card-body">' +
        badge(p) +
        '<h3><a href="' + esc(p.url) + '">' + esc(p.name) + "</a></h3>" +
        '<p class="sku">Item ' + esc(p.sku) + "</p>" +
        '<p class="tagline">' + esc(p.tagline) + "</p>" +
        (st.status === "coming-soon"
          ? (p.price != null ? '<div class="price price-expected">Expected: ' + esc(money(p.price)) + "</div>" : '<div class="price price-tba">Coming soon</div>')
          : priceHtml(p, false)) +
        action +
      "</div></article>";
  }

  function renderGrids() {
    $all("[data-grid]").forEach(function (el) {
      var kind = el.getAttribute("data-grid");
      var list = SHOP.products.slice();
      var p = params();
      if (kind === "featured") {
        var cat = p.get("cat");
        var q = (p.get("q") || "").trim().toLowerCase();
        if (cat) list = list.filter(function (x) { return x.category === cat; });
        if (q) list = list.filter(function (x) {
          return [x.name, x.sku, x.tagline, (x.highlights || []).join(" ")].join(" ").toLowerCase().indexOf(q) !== -1;
        });
        var title = $("[data-grid-title]");
        if (title) {
          if (q) title.textContent = 'Search results for "' + p.get("q") + '"';
          else if (cat) {
            var c = SHOP.categories.filter(function (c) { return c.id === cat; })[0];
            if (c) title.textContent = c.name;
          }
        }
        if (!q && !cat) list = list.filter(function (x) { return state(x).status !== "coming-soon"; });
        if (!list.length) { el.innerHTML = '<p class="empty">No products match. <a href="index.html">See all products</a>.</p>'; return; }
      } else if (kind === "coming-soon" || kind === "related") {
        list = list.filter(function (x) { return state(x).status === "coming-soon"; });
      } else if (kind === "category") {
        var id = el.getAttribute("data-category");
        list = list.filter(function (x) { return x.category === id; });
      }
      el.innerHTML = list.map(card).join("");
    });
  }

  /* ---------- product page ---------- */

  function renderGallery() {
    $all("[data-gallery]").forEach(function (el) {
      var p = product(el.getAttribute("data-gallery"));
      if (!p) return;
      el.innerHTML =
        '<div class="gallery-main"><img src="' + esc(p.images[0].src) + '" alt="' + esc(p.images[0].alt) + '"></div>' +
        '<p class="gallery-caption">' + esc(p.images[0].caption || "") + "</p>" +
        (p.images.length > 1 ? '<div class="gallery-thumbs">' + p.images.map(function (im, i) {
          return '<button type="button" class="thumb' + (i === 0 ? " is-active" : "") + '" data-i="' + i + '" aria-label="Show image ' + (i + 1) + '"><img src="' + esc(im.src) + '" alt=""></button>';
        }).join("") + "</div>" : "");
      var main = $(".gallery-main img", el);
      $all(".thumb", el).forEach(function (b) {
        b.addEventListener("click", function () {
          var im = p.images[+b.getAttribute("data-i")];
          main.src = im.src; main.alt = im.alt;
          $(".gallery-caption", el).textContent = im.caption || "";
          $all(".thumb", el).forEach(function (o) { o.classList.toggle("is-active", o === b); });
        });
      });
    });
  }

  /* ---------- free reservation form ---------- */

  function reserveBox(p) {
    return '<p class="buy-note">BeamForge is built in production runs. A run starts once enough boards are reserved. ' +
      "Reserving is free and commits you to nothing. When the run is confirmed, we email you a link to order at this price.</p>" +
      '<a class="btn btn-reserve" href="reserve.html">Reserve ' + esc(p.shortName || p.name) + '</a>';
  }

  function reserveForm(p) {
    if (/[?&]reserved=1\b/.test(location.search)) {
      return '<div class="reserve-done" role="status"><div class="reserve-check" aria-hidden="true">✓</div>' +
        "<h2>Your reservation is in</h2><p>Thanks! We'll email you when the production run is confirmed and ordering opens.</p>" +
        '<p class="reserve-fine">A confirmation is on its way to your inbox. <a href="beamforge.html">Back to BeamForge</a></p></div>';
    }
    if (!site.reserveFormAction) return '<p class="buy-note">Reservations open soon.</p>';
    var back = location.href.split("#")[0].split("?")[0] + "?reserved=1";
    var auto = "Thanks for reserving a " + p.name + ". Reserving is free and commits you to nothing. " +
      "When the production run is confirmed, we'll email you a link to order. " + (emailOk() ? "Questions: " + site.email : "");
    return '<form class="reserve" action="' + esc(site.reserveFormAction) + '" method="post">' +
        '<div class="reserve-head"><h2>Reserve now</h2><p>Free · no payment · no commitment</p></div>' +
        '<div class="reserve-body">' +
        '<input type="hidden" name="_subject" value="' + esc("Reservation: " + p.name) + '">' +
        '<input type="hidden" name="_template" value="table">' +
        '<input type="hidden" name="_next" value="' + esc(back) + '">' +
        '<input type="hidden" name="_autoresponse" value="' + esc(auto) + '">' +
        '<input type="hidden" name="product" value="' + esc(p.name + " (" + p.sku + ")") + '">' +
        '<input type="text" name="_honey" class="hp" tabindex="-1" autocomplete="off" aria-hidden="true">' +
        '<label><span>Email</span> <input type="email" name="email" required autocomplete="email" placeholder="you@example.com"></label>' +
        '<label><span>Name <span class="opt">(optional)</span></span> <input type="text" name="name" autocomplete="name"></label>' +
        '<div class="reserve-row">' +
          '<label><span>Ship to</span> <select name="ship_to" required>' +
            '<option value="">Choose…</option><option>Canada</option><option>United States</option><option>Europe</option><option>Other</option>' +
          "</select></label>" +
          '<label><span>Quantity</span> <input type="number" name="quantity" min="1" max="10" value="1" required></label>' +
        "</div>" +
        '<label><span>Comment <span class="opt">(optional)</span></span> <textarea name="comment" rows="2"></textarea></label>' +
        '<button class="btn reserve-submit" type="submit">Reserve ' + esc(p.shortName || p.name) + '</button>' +
        '<p class="reserve-fine">Your email is used only to tell you when ordering opens. <a href="policies.html#privacy">Privacy</a></p>' +
      "</div></form>";
  }

  function renderReserve() {
    var rates = SHOP.shipping.map(function (r) { return r.rate; }).filter(function (r) { return r != null; });
    $all("[data-ship-flat]").forEach(function (el) {
      el.textContent = rates.length ? money(Math.max.apply(null, rates)) : "";
    });
    $all("[data-reserve]").forEach(function (el) {
      var p = product(el.getAttribute("data-reserve"));
      if (p) el.innerHTML = reserveForm(p);
    });
  }

  function renderBuyBox() {
    $all("[data-buybox]").forEach(function (el) {
      var p = product(el.getAttribute("data-buybox"));
      if (!p) return;
      var st = state(p);
      var html =
        "<h1>" + esc(p.name) + "</h1>" +
        '<p class="sku">Item ' + esc(p.sku) + "</p>" +
        priceHtml(p, true) +
        (p.deposit != null ? '<p class="deposit">Deposit today: <b>' + esc(money(p.deposit)) + "</b>. The balance is due before shipping.</p>" : "") +
        '<div class="status-line">' + badge(p) + "</div>";

      if (st.status === "preorder-soon") {
        html += '<div class="countdown" data-countdown="' + esc(p.preorderOpens) + '" aria-live="polite"></div>' +
          '<p class="buy-note">Pre-orders open ' + esc(fmtDate(p.preorderOpens)) + ".</p>";
      } else if (st.buyable) {
        html += '<div class="buy" id="buy-' + esc(p.id) + '"></div>' +
          '<p class="buy-note">' + (st.status === "preorder"
            ? "You pay now and we ship when the batch is ready" + (site.shipWindow ? " (" + esc(site.shipWindow) + ")" : "") + ". Cancel any time before it ships for a full refund. <a href=\"policies.html#refunds\">Details</a>."
            : "Ships from " + esc(site.sellerCountry) + ". <a href=\"policies.html#shipping\">Shipping rates</a>.") + "</p>";
      } else if (st.status === "reserve") {
        html += reserveBox(p);
      } else if (st.status === "sold-out") {
        html += '<p class="buy-note">This batch is sold out.</p>';
      } else {
        html += '<p class="buy-note">Ordering isn\'t open yet.</p>';
      }

      if (p.highlights) html += '<ul class="highlights">' + p.highlights.map(function (h) { return "<li>" + esc(h) + "</li>"; }).join("") + "</ul>";
      if (st.status !== "reserve") html += '<div class="trust"><span class="paypal-mark small-mark" aria-label="PayPal"><b>Pay</b><i>Pal</i></span>' +
        "<span>Pay securely with PayPal: PayPal balance, or a credit or debit card without an account.</span></div>";
      el.innerHTML = html;
      if (st.buyable) paypalButton(p, $("#buy-" + p.id, el));
    });
  }

  var sdkLoading = null;
  function loadSdk() {
    if (sdkLoading) return sdkLoading;
    sdkLoading = new Promise(function (resolve, reject) {
      var s = document.createElement("script");
      s.src = "https://www.paypal.com/sdk/js?client-id=" + encodeURIComponent(SHOP.paypal.clientId) +
        "&components=hosted-buttons&disable-funding=venmo&currency=" + encodeURIComponent(site.currency);
      s.onload = function () { resolve(window.paypal); };
      s.onerror = function () { reject(new Error("PayPal SDK failed to load")); };
      document.head.appendChild(s);
    });
    return sdkLoading;
  }

  function paypalButton(p, el) {
    var cfg = SHOP.paypal;
    if (cfg.mode === "sdk" && cfg.clientId) {
      var cid = "paypal-container-" + p.paypalButtonId;
      el.innerHTML = '<div id="' + esc(cid) + '"></div>';
      loadSdk().then(function (paypal) {
        return paypal.HostedButtons({ hostedButtonId: p.paypalButtonId }).render("#" + cid);
      }).catch(function () {
        el.innerHTML = '<p class="error">The PayPal button could not load. Check your connection and reload, or <a href="faq.html#contact">contact us</a>.</p>';
      });
      return;
    }
    // Classic hosted button form: price, shipping and stock live at PayPal.
    var host = cfg.sandbox ? "https://www.sandbox.paypal.com" : "https://www.paypal.com";
    el.innerHTML =
      '<form action="' + host + '/cgi-bin/webscr" method="post" target="_top">' +
        '<input type="hidden" name="cmd" value="_s-xclick">' +
        '<input type="hidden" name="hosted_button_id" value="' + esc(p.paypalButtonId) + '">' +
        '<input type="hidden" name="currency_code" value="' + esc(site.currency) + '">' +
        '<button type="submit" class="btn btn-paypal">' + (state(p).status === "preorder" ? "Pre-order with " : "Buy with ") +
          '<span class="paypal-mark" aria-label="PayPal"><b>Pay</b><i>Pal</i></span></button>' +
      "</form>";
  }

  function renderSpecs() {
    $all("[data-specs]").forEach(function (el) {
      var p = product(el.getAttribute("data-specs"));
      if (!p || !p.specs) return;
      el.innerHTML = "<tbody>" + p.specs.map(function (r) {
        var todo = r[1] === "TODO";
        return '<tr><th scope="row">' + esc(r[0]) + "</th><td" + (todo ? ' class="tba"' : "") + ">" + (todo ? "To be announced" : esc(r[1])) + "</td></tr>";
      }).join("") + "</tbody>";
    });
    $all("[data-inbox]").forEach(function (el) {
      var p = product(el.getAttribute("data-inbox"));
      if (!p || !p.inTheBox) return;
      el.innerHTML = p.inTheBox.map(function (x) {
        return /^TODO/.test(x) ? "" : "<li>" + esc(x) + "</li>";
      }).join("") + (p.inTheBox.some(function (x) { return /^TODO/.test(x); }) ? '<li class="tba">Full contents to be announced</li>' : "");
    });
  }

  function renderShipping() {
    $all("[data-shipping-table]").forEach(function (el) {
      el.innerHTML = "<thead><tr><th>Ships to</th><th>Carrier</th><th>Flat rate</th></tr></thead><tbody>" +
        SHOP.shipping.map(function (r) {
          return "<tr><td>" + esc(r.region) + "</td><td>" + esc(r.carrier) + "</td><td" + (r.rate == null ? ' class="tba"' : "") + ">" +
            (r.rate == null ? "To be announced" : esc(money(r.rate))) + "</td></tr>";
        }).join("") + "</tbody>";
    });
  }

  /* ---------- small bits ---------- */

  function fillSite() {
    $all("[data-site]").forEach(function (el) {
      var k = el.getAttribute("data-site");
      if (k === "email") {
        if (emailOk()) el.innerHTML = '<a href="mailto:' + esc(site.email) + '">' + esc(site.email) + "</a>";
        else el.textContent = "(contact email to be announced)";
      } else if (k === "shipWindow") {
        el.textContent = site.shipWindow || "to be announced";
      } else if (k === "preorderOpens") {
        var p = product("beamforge");
        el.textContent = p && p.preorderOpens ? fmtDate(p.preorderOpens) : "November 2026";
      } else if (site[k] != null) {
        el.textContent = site[k];
      }
    });
    $all("[data-price]").forEach(function (el) {
      var p = product(el.getAttribute("data-price"));
      el.textContent = p && p.price != null ? money(p.price) : "Price announced soon";
    });
    $all("[data-badge]").forEach(function (el) {
      var p = product(el.getAttribute("data-badge"));
      if (p) el.innerHTML = badge(p);
    });
    $all("[data-countdown-product]").forEach(function (el) {
      var p = product(el.getAttribute("data-countdown-product"));
      if (p && p.preorderOpens) el.setAttribute("data-countdown", p.preorderOpens);
    });
    $all("[data-show-when]").forEach(function (el) {
      var bits = el.getAttribute("data-show-when").split(":");
      var p = product(bits[0]);
      el.hidden = !p || bits.slice(1).indexOf(state(p).status) === -1;
    });
  }

  function startCountdowns() {
    var els = $all("[data-countdown]");
    if (!els.length) return;
    var offset = now().getTime() - Date.now(); // keeps a faked date ticking
    function tick() {
      var t = Date.now() + offset;
      els.forEach(function (el) {
        var ms = parseDate(el.getAttribute("data-countdown")).getTime() - t;
        if (ms <= 0) { el.innerHTML = '<span class="cd-open">Pre-orders are open. Reload the page.</span>'; return; }
        var d = Math.floor(ms / 864e5), h = Math.floor(ms / 36e5) % 24, m = Math.floor(ms / 6e4) % 60, s = Math.floor(ms / 1e3) % 60;
        el.innerHTML = [[d, "days"], [h, "hours"], [m, "min"], [s, "sec"]].map(function (x) {
          return '<span class="cd-cell"><b>' + (x[0] < 10 ? "0" : "") + x[0] + "</b><small>" + x[1] + "</small></span>";
        }).join("");
      });
    }
    tick();
    setInterval(tick, 1000);
  }

  function initTabs() {
    $all(".tabs").forEach(function (box) {
      var tabs = $all('[role="tab"]', box);
      function show(tab, focus) {
        tabs.forEach(function (t) {
          var on = t === tab;
          t.setAttribute("aria-selected", on ? "true" : "false");
          t.tabIndex = on ? 0 : -1;
          document.getElementById(t.getAttribute("aria-controls")).hidden = !on;
        });
        if (focus) tab.focus();
      }
      tabs.forEach(function (t, i) {
        t.addEventListener("click", function () { show(t); history.replaceState(null, "", "#" + t.getAttribute("aria-controls")); });
        t.addEventListener("keydown", function (e) {
          var j = e.key === "ArrowRight" ? i + 1 : e.key === "ArrowLeft" ? i - 1 : null;
          if (j == null) return;
          e.preventDefault();
          show(tabs[(j + tabs.length) % tabs.length], true);
        });
      });
      function fromHash() {
        var h = location.hash.slice(1);
        var t = tabs.filter(function (t) { return t.getAttribute("aria-controls") === h; })[0];
        show(t || tabs[0]);
        if (t) box.scrollIntoView();
      }
      window.addEventListener("hashchange", fromHash);
      fromHash();
      // Links elsewhere on the page that open a tab.
      $all('a[data-tab]').forEach(function (a) {
        a.addEventListener("click", function (e) {
          var t = tabs.filter(function (t) { return t.getAttribute("aria-controls") === a.getAttribute("data-tab"); })[0];
          if (t) { e.preventDefault(); show(t); box.scrollIntoView({ behavior: "smooth" }); }
        });
      });
    });
  }

  document.addEventListener("DOMContentLoaded", function () {
    renderHeader();
    renderCrumbs();
    renderSidebar();
    renderFooter();
    renderGrids();
    renderGallery();
    renderBuyBox();
    renderReserve();
    renderSpecs();
    renderShipping();
    fillSite();
    initTabs();
    startCountdowns();
  });
})();
