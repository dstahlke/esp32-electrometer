// Based on:
// https://www.donskytech.com/real-time-sensor-chart-display-of-sensor-readings-esp8266-esp32/

var targetUrl = `ws://${window.location.hostname}/ws`;

var n = 60;
var data = d3.range(n).map(i => 0.5)

var svg = d3.select("svg"),
    margin = {top: 20, right: 20, bottom: 20, left: 40},
    width = +svg.attr("width") - margin.left - margin.right,
    height = +svg.attr("height") - margin.top - margin.bottom,
    g = svg.append("g").attr("transform", "translate(" + margin.left + "," + margin.top + ")");

var x = d3.scaleLinear()
    .domain([0, n - 1])
    .range([0, width]);

var y = d3.scaleLinear()
    .domain([0, 1])
    .range([height, 0]);

var line = d3.line()
    .x(function(d, i) { return x(i); })
    .y(function(d, i) { return y(d); });

g.append("defs").append("clipPath")
    .attr("id", "clip")
    .append("rect")
    .attr("width", width)
    .attr("height", height);

g.append("g")
    .attr("class", "yaxis")
    .call(d3.axisLeft(y));

g.append("g")
    .attr("clip-path", "url(#clip)")
    .attr("fill", "none")
    .attr("stroke", "steelblue")
    .attr("stroke-width", 3)
    .append("path")
    .datum(data)
    .attr("class", "line");


var websocket;

window.addEventListener("load", onLoad);

function onLoad() {
    initializeSocket();
}

function initializeSocket() {
    console.log(`Opening WebSocket connection to Microcontroller :: ${targetUrl}`);
    websocket = new WebSocket(targetUrl);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}
function onOpen(event) {
    console.log("Starting connection to server..");
}
function onClose(event) {
    console.log("Closing connection to server..");
    setTimeout(initializeSocket, 2000);
}
function onMessage(event) {
    console.log("WebSocket message received:", event);
    var val = event.data / 0x7fffff;
    $('#valbox').text(val.toFixed(6))

    data.push(val);
    data.shift();

    y.domain([d3.min(data), d3.max(data)]);

    svg.select(".yaxis")
        .call(d3.axisLeft(y));

    svg.select(".line")
        .attr("d", line)
        .attr("transform", null);
}
