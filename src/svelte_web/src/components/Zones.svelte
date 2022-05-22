<script>
  export let zones;
  import { createEventDispatcher } from "svelte";

  const dispatch = createEventDispatcher();

  function ClickSoftButton(zone) {
    dispatch("softbutton", { zone: zone });
  }

  /*
    AUDIBLE,
    SILENT,
    PULSED,
    NONE
    */
  let attributes = [`Audible`, `Silent`, `Pulsed`, `None`];
  /*
  NO_USED,                // The zone will not operate in any way. Zones that are not used should be programmed as Null zones
    DELAY,                  // If this zone is violated when the panel is armed it will provide entry delay
    INSTANT,                // If this zone type is violated when the panel is armed it will cause an instant alarm. Typically this zone is used for windows, patio doors or other perimeter type zones
    INTERIOR,               // If this type of zone is violated when the panel is armed it will provide entry if a delay type zone was violated first. Otherwise it will cause an instant alarm. Typically this zone is used for interior protection devices, such as motion detectors.
    ALWAYS_ARMED,           // If this type of zone is violated when the panel is armed or disarmed it will cause an instant alarm. Typically this zone is used for 24 hour zones.
    KEYSWITCH_ARM,          // When this zone is violated, the system will arm. When this zone is secured, the system will disarm.
    MOMENTARY_KEYSWITCH_ARM // Momentary violation of this zone will alternately arm/disarm the system.
  */
  let definitions = [
    "No Used",
    "Delay",
    "Instant",
    "Interior",
    "24 Hours",
    "KeySwitch Arm",
    "Momentary KeySwitch Arm",
  ];

  let sensorTypes = ["Normally Open", "Normally Closed", "Soft Button"];
  /*
    TROUBLE,
    NORMAL,
    ALARM,
    UNDEFINED
  */
  let ZoneStatus = ["Trouble", "Normal", "Alarm", "Undefined"];
</script>

<h2>Zone Status</h2>
<p>General status of the alarm zones</p>

<table>
  <tr>
    <th>GPIO</th>
    <th>Label</th>
    <th>Status</th>
    <th>Sensor Type</th>
    <th>Definition</th>
    <th>Chime</th>
    <th>Attributes</th>
    <th>Memory</th>
    <th>Function</th>
  </tr>
  {#if zones}
    {#each zones as zone, i}
      <tr>
        <td>{zone.gpio}</td>
        <td>{zone.zone_label}</td>
        <td>
          {#if zone.status == 0}
            <span class="on_trouble"> &#9888; {ZoneStatus[zone.status]}</span>
          {:else if zone.status == 1}
            <span class="on_normal"> &#10033; {ZoneStatus[zone.status]}</span>
          {:else if zone.status == 2}
            <span class="on_alarm"> &#9873; {ZoneStatus[zone.status]}</span>
          {:else}
            <span class="">{ZoneStatus[zone.status]}</span>
          {/if}
        </td>
        <td>
          {sensorTypes[zone.sensor_type]}
        </td>
        <td>
          {definitions[zone.definition]}
        </td>
        {#if zone.chime}
          <td class="on_memory_alarm">&#10004;</td>
        {:else}
          <td class="on_no_memory_alarm">&#10004;</td>
        {/if}

        <td>
          {attributes[zone.attributes]}
        </td>

        {#if zone.memory_alarm}
          <td class="on_memory_alarm">&#10033;</td>
        {:else}
          <td class="on_no_memory_alarm">&#10033;</td>
        {/if}

        {#if zone.sensor_type == 2}
          <td
            ><button
              type="button"
              on:mouseup={(e) => {
                zone.status = 1;
                ClickSoftButton(zone);
              }}
              on:click={(e) => {
                zone.status = 2;
                ClickSoftButton(zone);
              }}>Panic</button
            ></td
          >
        {:else}
          <td />
        {/if}
      </tr>
    {/each}
  {/if}
</table>

<style>
  table {
    border-collapse: collapse;
    width: 100%;
    width: -webkit-fill-available;
  }

  th,
  td {
    padding: 8px;
    text-align: left;
    border-bottom: 1px solid #ddd;
  }

  tr:hover {
    background-color: #d6eeee;
  }

  .on_no_memory_alarm {
    color: dimgrey;
    text-align: center;
  }
  .on_memory_alarm {
    color: rgb(255, 172, 47);
    text-align: center;
  }

  .on_trouble {
    color: rgb(243, 138, 0);
  }
  .on_normal {
    color: rgb(43, 153, 0);
  }
  .on_alarm {
    color: rgb(218, 44, 0);
  }
</style>
