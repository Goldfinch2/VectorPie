# BeamForge Shop website

A static store for BeamForge and the cabinet adapters: plain HTML, CSS and
JavaScript, no build step, no server. Payment is PayPal hosted buttons, so
prices, shipping and stock limits live in your PayPal account, not in the page.

```
website/
  index.html       store front: banner, featured products, coming soon
  beamforge.html   product page: gallery, buy box, tabs, related products
  adapters.html    Asteroids and Space Duel adapters
  faq.html         pre-orders, my order, shipping, refunds, support, contact
  policies.html    shipping, refund and privacy policies
  css/style.css
  js/products.js   THE config file: products, prices, PayPal, shipping, email
  js/site.js       shared header/sidebar/footer and all rendering
  assets/          logo and vector-style illustrations (replace with photos)
```

## Before launch: fill in the TODOs

Everything is in `js/products.js`. A `null` shows as "announced soon" or
"to be announced" on the site, never as a made-up value.

| Field | What |
|---|---|
| `site.email` | Contact address |
| `site.shipWindow` | e.g. `"Q2 2027"` |
| `paypal.clientId` | PayPal client ID (only for `mode: "sdk"`) |
| `shipping[].rate` | Flat USD rate per region (same as in PayPal) |
| `products[].price` | USD price, PayPal fee built in |
| `products[].deposit` | Optional deposit instead of the full price |
| `products[].paypalButtonId` | Hosted button ID |
| BeamForge `specs` / `inTheBox` | The `TODO` rows (monitor connection, power, dimensions, weight, contents) |

Also review the policy text in `policies.html`: the 30-day return window, the
refused-parcel rule and the duties wording are proposed defaults, not
decisions. Check the US entry rules for goods from Canada (CUSMA origin
marking) with Canada Post or a customs broker before shipping to the US.

## PayPal

### Pricing

Build the fee into the price; don't add a surcharge line. Seller in Canada,
USD only (no currency conversion): about 2.9% + a fixed fee, plus about 0.8%
for US buyers and about 1% for buyers elsewhere. Check PayPal's live fee page
for Canada before setting the price.

### Create a button

1. PayPal Business account > **Pay & Get Paid > PayPal buttons** (or
   **Tools > PayPal Buttons**). Create a **Buy Now** button.
2. Item name `BeamForge Vector Generator`, item ID `BF-001`, price, USD.
3. Shipping: flat rate per region, or use your account shipping settings
   (Settings > Shipping), with the same rates as `shipping` in `products.js`.
   Ship to Canada, the US and European countries only.
4. **Inventory**: turn on tracking and set the batch size, so the button stops
   selling when the batch is full. Allow buyers to choose quantity if you want
   more than one per order (quantity is chosen at PayPal's checkout).
5. Save. PayPal shows embed code. Copy the button ID:
   - new style: `paypal.HostedButtons({ hostedButtonId: "ABC123" })` and a
     `client-id=...` in the script URL. Use `paypal.mode: "sdk"` and paste the
     client ID in `paypal.clientId`.
   - classic style: `<input name="hosted_button_id" value="ABC123">`. Use
     `paypal.mode: "form"`.
6. Paste the ID in the product's `paypalButtonId`.

In PayPal > Settings, also set your refund policy and a return address.

### Test with the sandbox first

At developer.paypal.com, create a sandbox business and a sandbox personal
account. Log into sandbox.paypal.com as the business, create the same button,
and put its ID in `paypalButtonId`. With `mode: "form"` set `sandbox: true`;
with `mode: "sdk"` use the **Sandbox** client ID. Buy with the personal
account. Switch back to the live ID / client ID (and `sandbox: false`) after.

### Pre-order cautions

- PayPal buyers can open a dispute for up to 180 days. Keep the ship window
  honest and email buyers if it slips.
- New seller accounts can have funds held until delivery is confirmed. Plan
  your production cash for that.
- A deposit (`deposit` field, a separate PayPal button for the deposit amount)
  lowers both risks.

## Changing what's on sale

Each product has a `status`:

`coming-soon` → `preorder` → `available` → `sold-out`

- `coming-soon`: greyed card, red COMING SOON badge. Never a buy button.
- `preorder` with `preorderOpens`: before that date, a countdown; from that date
  on (the visitor's local midnight), the PayPal button. No
  redeploy needed on the day.
- `preorder` / `available` without a `paypalButtonId`: "Ordering isn't open
  yet" instead of a button.
- `sold-out`: no button.

To put the Asteroids adapter on pre-order, for example: set `status:
"preorder"`, `price`, `paypalButtonId`, and (optionally) `preorderOpens`.

## Run it locally

```
python3 -m http.server -d website 8000
```

Open http://localhost:8000. On localhost only, `?now=2026-12-02` fakes the
date, to see the page after pre-orders open:
http://localhost:8000/beamforge.html?now=2026-12-02

## Deploy

**GitHub Pages:** `.github/workflows/pages.yml` publishes `website/` on every
push to `main` that touches it. Once: repo **Settings > Pages > Source: GitHub
Actions**. For a custom domain, add it under Settings > Pages and create the
DNS records GitHub shows; tick **Enforce HTTPS**.

**Cloudflare Pages (alternative):** create a Pages project from the repo,
framework preset "None", build command empty, output directory `website`. Add
the custom domain in the project's settings.

## Trademarks

No Atari logos or artwork anywhere. Cabinet names are only used to describe
compatibility; the footer carries the trademark disclaimer.
