#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>
#include <WiFiS3.h>

extern int deviceStats[7];   // deviceStats[1..5]
extern bool showStatsScreen; // handle the "Finished" state
extern int timePassed;

void sendHTML(WiFiClient client) {
  client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  client.println("<title>Arduino Device Stats</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; background: #f4f6f9; color: #333; text-align: center; margin: 0; padding: 20px; }");
  client.println("h1 { color: #0066cc; margin-bottom: 20px; }");
  client.println(".card { background: white; max-width: 400px; margin: auto; padding: 20px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }");
  client.println(".status { display: flex; align-items: center; justify-content: center; margin-bottom: 20px; font-weight: bold; }");
  client.println(".status-circle { width: 16px; height: 16px; border-radius: 50%; display: inline-block; margin-right: 10px; }");
  client.println(".status-off { background-color: #f1df19ff; }");       // red
  client.println(".status-on { background-color: #4CAF50; }");        // green
  client.println(".status-finished { background-color: #000dffff; }");  // orange
  client.println("p { font-size: 16px; margin: 8px 0; }");
  client.println("form { margin: 15px 0; }");
  client.println("input[type=number] { padding: 8px; width: 120px; border: 1px solid #ccc; border-radius: 6px; }");
  client.println("input[type=button], button { padding: 10px 18px; margin: 5px; border: none; border-radius: 6px; background: #0066cc; color: white; font-size: 14px; cursor: pointer; }");
  client.println("input[type=button]:hover, button:hover { background: #004c99; }");
  client.println("</style></head><body>");

  client.println("<div class='card'>");
  client.println("<h1>Device Stats</h1>");

  // Status indicator
  String statusClass, statusText;
  if (!showStatsScreen) {
    statusClass = "status-finished";
    statusText = "Finished";
  } else if (deviceStats[5]) {
    statusClass = "status-on";
    statusText = "On";
  } else {
    statusClass = "status-off";
    statusText = "Off";
  }

  client.println("<div class='status'>");
  client.println("<span id='statusCircle' class='status-circle " + statusClass + "'></span>");
  client.println("<span id='statusText'>" + statusText + "</span>");
  client.println("</div>");

  // Stats display
  client.println("<p><b>Desired Current:</b> <span id='desiredCurrent'>" + String(deviceStats[2]) + "</span> mA</p>");
  client.println("<p><b>Measured Current:</b> <span id='measuredCurrent'>" + String(deviceStats[3]) + "</span> mA</p>");
  client.println("<p><b>Load Voltage:</b> <span id='loadVoltage'>" + String(deviceStats[4]) + "</span> mV</p>");
  client.println("<p><b>Load Resistance:</b> <span id='loadResistance'>" + String(deviceStats[6]) + "</span> ohm</p>");
  client.println("<p><b>Time:</b> <span id='time'>" + String(timePassed) + " / " + String(deviceStats[1]) + "</span> s</p>");

  // Form to update current
  client.println("<form onsubmit='return false;'>");
  client.println("<label>Set Current (mA): </label><br>");
  client.println("<input type='number' name='currentInput' id='currentInput'><br>");
  client.println("<input type='button' value='Update' onclick='updateCurrent()'>");
  client.println("</form>");

  // Form to update time
  client.println("<form onsubmit='return false;'>");
  client.println("<label>Set Time (s): </label><br>");
  client.println("<input type='number' name='timeInput' id='timeInput'><br>");
  client.println("<input type='button' value='Update' onclick='updateTime()'>");
  client.println("</form>");

  // Start/Stop and Restart buttons
  client.println("<form onsubmit='return false;' style='display:inline;'>");
  client.println("<button type='button' name='run' onclick='toggleRun()'>Start/Stop</button>");
  client.println("</form>");
  client.println("<form onsubmit='return false;' style='display:inline;'>");
  client.println("<button type='button' name='run' onclick='restartDevice()'>Restart</button>");
  client.println("</form>");
  client.println("<form onsubmit='return false;' style='display:inline;'>");
  client.println("</form>");

  client.println("</div>"); // end card

  // JavaScript to auto-update stats
  client.println("<script>");
  client.println("let isFetching = false;");
  client.println("function updateStats() {");
  client.println("  if (isFetching) return;");
  client.println("  isFetching = true;");
  client.println("  fetch('/stats')");
  client.println("    .then(response => response.json())");
  client.println("    .then(data => {");
  client.println("      document.getElementById('desiredCurrent').innerText = data.desiredCurrent;");
  client.println("      document.getElementById('measuredCurrent').innerText = data.measuredCurrent;");
  client.println("      document.getElementById('loadVoltage').innerText = data.loadVoltage;");
  client.println("      document.getElementById('time').innerText = data.time;");
  client.println("      document.getElementById('loadResistance').innerText = data.loadResistance;");
  client.println("      const statusCircle = document.getElementById('statusCircle');");
  client.println("      const statusText = document.getElementById('statusText');");
  client.println("      statusCircle.className = 'status-circle';");
  client.println("      if (!data.showStatsScreen) { statusCircle.classList.add('status-finished'); statusText.innerText = 'Finished'; }");
  client.println("      else if (data.deviceOn) { statusCircle.classList.add('status-on'); statusText.innerText = 'On'; }");
  client.println("      else { statusCircle.classList.add('status-off'); statusText.innerText = 'Off'; }");
  client.println("    })");
  client.println("    .catch(error => console.error('Fetch error:', error))");
  client.println("    .finally(() => { isFetching = false; });");
  client.println("}");
  client.println("window.onload = function() { setInterval(updateStats, 100); };");
  client.println("function updateCurrent() {");
  client.println("  const current = document.getElementById('currentInput').value;");
  client.println("  if (current) fetch('/updateCurrent?value=' + current);");
  client.println("}");
  client.println("function updateTime() {");
  client.println("  const time = document.getElementById('timeInput').value;");
  client.println("  if (time) fetch('/updateTime?value=' + time);");
  client.println("}");
  client.println("function toggleRun() { fetch('/toggleRun'); }");
  client.println("function restartDevice() { fetch('/restart'); }");
  client.println("</script>");

  client.println("</body></html>");
}

#endif
