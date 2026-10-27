// Browser subscription bridge for Yamaha EZ -> ESP -> Editor
// Connects via WebSocket to ESP; creates a virtual MIDI feed for strudeleditor.
// If navigator.requestMIDIAccess is available, this can feed it via a polyfill.
(function(){
  const ws = new WebSocket(`ws://${location.hostname}:80/midi`);
  window.yamahaMIDI = { notes: {}, send: null };

  ws.onopen = () => {
    console.log("Subscribed to ESP MIDI bridge");
    // trigger on access / subscription
    window.dispatchEvent(new CustomEvent("yamahaMIDIReady"));
  };

  ws.onmessage = (e) => {
    const data = e.data;
    if (!data) return;
    // Simple parser for msg format from ESP: m:90:3C:7F
    if (data.startsWith("m:")) {
      const [b1,b2,b3] = data.split(":").slice(1).map(x=>parseInt(x,16));
      const status = b1 & 0xF0;
      const note = b2;
      const vel = b3;
      // Push to any listener (strudeleditor hook)
      window.dispatchEvent(new CustomEvent("yamahaNote", {
        detail: { status, note, vel, raw: [b1,b2,b3] }
      }));
    }
  };

  ws.onclose = () => console.log("ESP MIDI bridge disconnected");
})();
