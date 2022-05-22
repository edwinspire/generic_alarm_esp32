<script>
  import ZonesComponent from "./components/Zones.svelte";
  import { onMount } from "svelte/internal";
  let wsData = [];
  var gateway = "";
  var websocket;
  let wsRequest = {};

  /*
  DISARMED,     // The system is disarmed and ready to be armed
  ARMED,        // The system is armed and ready to go
  ARMED_FORCED, // System armed but with some zone open or in trouble
  UNDEFINED
*/
  let armed_status = ["Disarmed", "Armed", "Armed Forced", "Undefined"];

  function onLoad(event) {
    initWebSocket();
  }

  function Arm_Disarm() {
    //websocket.send("toggle");
    //wsRequest.armed_status =
    console.log(wsData);
    if (wsData.armed_status == 0) {
      wsRequest.armed_status = 1;
    } else {
      wsRequest.armed_status = 0;
    }
    websocket.send(JSON.stringify(wsRequest));
  }

  function initWebSocket() {
    console.log("Trying to open a WebSocket connection...");
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log("Connection opened");
  }
  function onClose(event) {
    console.log("Connection closed");
    setTimeout(initWebSocket, 30000);
  }
  function onMessage(event) {
    //console.log('Message received: ' + event.data);
    try {
      wsData = JSON.parse(event.data);
      console.log(wsData);
    } catch (error) {
      console.error(error, event.data);
    }
  }

  onMount(async () => {
    gateway = `ws://${window.location.hostname}/ws`;
    setTimeout(() => {
      wsData = {
        armed_status: 0,
        trouble_zone: 0,
        alarm_audible: 0,
        alarm_pulsed: 0,
        alarm_silent: 0,
        alarm_zone: 0,
        alarm_memory: 0,
        zones: [
          {
            memory_alarm: false,
            zone_label: "Zone 01",
            gpio: 34,
            status: 0,
            sensor_type: 0,
            definition: 2,
            attributes: 0,
            chime: true,
          },
          {
            memory_alarm: true,
            zone_label: "Zone 02",
            gpio: 35,
            status: 1,
            sensor_type: 0,
            definition: 4,
            attributes: 2,
            chime: false,
          },
        ],
      };
    }, 5000);
    onLoad();
  });
</script>

<div class="topnav">
  <h1>ESP32 WebSocket Server</h1>
</div>
<div class="content">
  <div class="card">
    <h2>Arm Status</h2>
    <div>{armed_status[wsData.armed_status]}</div>
    <div class="switch_container">
      <label class="switch">
        <input
          type="checkbox"
          checked={wsData.armed_status != 0}
          on:change={Arm_Disarm}
        />
        <span class="slider " />
      </label>
    </div>

    {#if wsData.output_02 && wsData.output_02 > 0}
      <p>&#128266; Actived</p>
    {:else}
      <p>&#128264; Off</p>
    {/if}
  </div>

  <div class="card">
    <ZonesComponent
      bind:zones={wsData.zones}
      on:softbutton={(e) => {
        console.log(e);
        websocket.send(
          JSON.stringify({ zone: e.detail.zone, action: "softbutton" })
        );
      }}
    />
  </div>
</div>

<style>
  h1 {
    font-size: 1.8rem;
    color: white;
    text-align: center;
  }

  h2 {
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
    text-align: center;
  }

  .topnav {
    overflow: hidden;
    background-color: #00212c;
  }

  .card {
    background-color: #f8f7f9;
    box-shadow: 2px 2px 12px 1px rgb(140 140 140 / 50%);
    padding: 1.5em;
    margin: 1.5em;
  }

  .switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
  }

  .switch input {
    opacity: 0;
    width: 0;
    height: 0;
  }

  .slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    -webkit-transition: 0.4s;
    transition: 0.4s;
  }

  .slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background-color: white;
    -webkit-transition: 0.4s;
    transition: 0.4s;
  }

  input:checked + .slider {
    background-color: #da0101;
  }

  input:checked + .slider:before {
    -webkit-transform: translateX(26px);
    -ms-transform: translateX(26px);
    transform: translateX(26px);
  }

  .content {
    padding: 30px;
    max-width: 75%;
    margin: 0 auto;
  }
  .switch_container {
    text-align: center;
  }
  h1 {
    color: #ff3e00;
    text-transform: uppercase;
    font-size: 4em;
    font-weight: 100;
  }
</style>
