// Control page served at "/". Kept in a header so the Arduino prototype
// generator never tries to parse the JavaScript. The browser does the image
// work: it scales the chosen picture to 400 x 300, dithers it down to black,
// white and red, and uploads the two 1 bit planes the sketch stores as is.
const char pageHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="robots" content="noindex">
<link rel="icon" href="data:,">
<meta name="description" content="Change the notes and images shown on the e-paper display.">
<title>E-paper display control</title>
<style>
body{font-family:system-ui,sans-serif;max-width:440px;margin:0 auto;padding:16px;color:#111;background:#fff}
h1{font-size:1.4rem}
h2{font-size:1.1rem;margin-top:2rem}
label{display:block;margin:12px 0 4px}
textarea,select,input,button{font:inherit;box-sizing:border-box}
input[type=range]{width:100%}
textarea{width:100%}
button{padding:8px 16px;margin:8px 8px 0 0}
canvas{width:100%;height:auto;border:1px solid #111;margin-top:12px}
#status{margin-top:16px;font-weight:bold}
</style>
</head>
<body>
<h1>E-paper display</h1>
<p>A change shows on the panel after about 10 seconds of flashing.</p>

<h2>Notes</h2>
<label for="notes">One note per line, up to 5 lines of about 30 characters</label>
<textarea id="notes" rows="5" maxlength="400"></textarea>
<button id="save">Save notes</button>

<h2>Images</h2>
<label for="slot">Screen to replace</label>
<select id="slot">
<option value="1">Slot 1 (forecast)</option>
<option value="2">Slot 2 (events)</option>
<option value="3">Slot 3 (notes)</option>
</select>
<label for="file">Picture</label>
<input id="file" type="file" accept="image/*">
<canvas id="preview" width="400" height="300" role="img" aria-label="Preview of the picture as the panel will show it"></canvas>
<button id="upload" disabled>Upload to slot</button>
<button id="remove">Remove image from slot</button>

<h2>Rotation</h2>
<label for="minutes">Minutes per screen (1 to 120, the panel maker advises 3 or more)</label>
<input id="minutes" type="number" min="1" max="120" step="1">
<label for="screens">Screens in the rotation: <output id="screensLabel" for="screens"></output></label>
<input id="screens" type="range" min="1" max="4" step="1">
<button id="apply">Save rotation</button>

<p id="status" role="status"></p>

<script>
const $ = id => document.getElementById(id);
const say = text => $('status').textContent = text;
const palette = [[0, 0, 0], [255, 255, 255], [200, 0, 0]];
const umlauts = {'ä': 'ae', 'ö': 'oe', 'ü': 'ue', 'Ä': 'Ae', 'Ö': 'Oe', 'Ü': 'Ue', 'ß': 'ss'};
let planes = null;

const screenLabels = ['', '1, clock only', '2, clock and slot 1', '3, clock and slots 1 to 2', '4, clock and slots 1 to 3'];
const showScreens = () => $('screensLabel').textContent = screenLabels[$('screens').value];

fetch('/notes').then(r => r.text()).then(text => $('notes').value = text);
fetch('/settings').then(r => r.json()).then(saved => {
  $('minutes').value = saved.minutes;
  $('screens').value = saved.screens;
  showScreens();
});
$('screens').oninput = showScreens;

$('apply').onclick = async () => {
  const body = new URLSearchParams({minutes: $('minutes').value, screens: $('screens').value});
  const reply = await fetch('/settings', {method: 'POST', body});
  say(reply.ok ? 'Rotation saved, it applies from the next screen change.' : 'Saving failed: ' + await reply.text());
};

// The panel font only has plain ASCII, so umlauts are spelled out
$('save').onclick = async () => {
  const text = $('notes').value.replace(/[äöüÄÖÜß]/g, c => umlauts[c]).replace(/[^\x20-\x7e\n]/g, '');
  $('notes').value = text;
  const reply = await fetch('/notes', {method: 'POST', body: new URLSearchParams({text})});
  say(reply.ok ? 'Notes saved, the panel is refreshing.' : 'Saving failed: ' + await reply.text());
};

// Scales the picture to cover 400 x 300, then Floyd Steinberg dithers it to
// the three panel colours and packs the black and red planes
$('file').onchange = async event => {
  if (!event.target.files[0]) return;
  const picture = await createImageBitmap(event.target.files[0]);
  const context = $('preview').getContext('2d');
  const scale = Math.max(400 / picture.width, 300 / picture.height);
  const width = picture.width * scale, height = picture.height * scale;
  context.fillStyle = '#fff';
  context.fillRect(0, 0, 400, 300);
  context.drawImage(picture, (400 - width) / 2, (300 - height) / 2, width, height);

  const image = context.getImageData(0, 0, 400, 300), pixels = image.data;
  planes = new Uint8Array(30000);
  for (let y = 0; y < 300; y++) {
    for (let x = 0; x < 400; x++) {
      const i = (y * 400 + x) * 4;
      let best = 0, bestDistance = Infinity;
      palette.forEach((colour, index) => {
        const distance = (pixels[i] - colour[0]) ** 2 + (pixels[i + 1] - colour[1]) ** 2 + (pixels[i + 2] - colour[2]) ** 2;
        if (distance < bestDistance) { bestDistance = distance; best = index; }
      });
      for (let channel = 0; channel < 3; channel++) {
        const error = pixels[i + channel] - palette[best][channel];
        pixels[i + channel] = palette[best][channel];
        if (x < 399) pixels[i + 4 + channel] += error * 7 / 16;
        if (y < 299) {
          if (x > 0) pixels[i + 1596 + channel] += error * 3 / 16;
          pixels[i + 1600 + channel] += error * 5 / 16;
          if (x < 399) pixels[i + 1604 + channel] += error / 16;
        }
      }
      if (best !== 1) planes[(best === 2 ? 15000 : 0) + y * 50 + (x >> 3)] |= 0x80 >> (x & 7);
    }
  }
  context.putImageData(image, 0, 0);
  $('upload').disabled = false;
  say('Preview ready.');
};

$('upload').onclick = async () => {
  const form = new FormData();
  form.append('image', new Blob([planes]), 'image.bin');
  const reply = await fetch('/slot/' + $('slot').value, {method: 'POST', body: form});
  say(reply.ok ? 'Image saved, the panel is refreshing.' : 'Upload failed: ' + await reply.text());
};

$('remove').onclick = async () => {
  const reply = await fetch('/slot/' + $('slot').value, {method: 'DELETE'});
  say(reply.ok ? 'Image removed, the panel is refreshing.' : 'Removing failed: ' + await reply.text());
};
</script>
</body>
</html>
)rawliteral";
