let here = new URL(location.protocol + '//' + location.host + location.pathname);

var checkboxElems = document.querySelectorAll("input[type='checkbox']");
var keep_color_elem = document.querySelector("#keep_color");
var random_color_elem = document.querySelector("#random_color");

const modes = ["Static", "Slow Flash", "Quick Flash", "Breathing Mode", "Combo Flash", "All (set 1)", "XMAS vibe", "Sunshine", "Forest", "Nuclear Fallout", "All (set 2)"];

//document.querySelector("#curr_mode").innerHTML = "Current: " + modes[%CURRENT_PATTERN%];

// add event listeners
for (var i = 0; i < checkboxElems.length; i++) {
    checkboxElems[i].addEventListener("click", set_config);
}

keep_color_elem.addEventListener("change", set_config);
random_color_elem.addEventListener("change", set_config);

function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? "0" + hex : hex;
  }

function rgbToHex(r, g, b) {
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

document.querySelector("#color-picker").value = rgbToHex(%R%, %G%, %B%);

function set_color(r, g, b) {
    //document.getElementById("change_color").href = "?r=" + Math.round(r) + "&g=" + Math.round(g) + "&b=" + Math.round(b);

    here.searchParams.append("r", r);
    here.searchParams.append("g", g);
    here.searchParams.append("b", b);
    window.location.href = here.href;
}

function set_mode(mode_id) {
    here.searchParams.append("mode", mode_id);
    window.location.href = here.href;
}

function set_config(e) {
    let timeout = 0;

    if (e.target.id == "motion_detect") {
        timeout = document.querySelector("#timeout_s").value;

        here.searchParams.append("timeout", timeout);
    }

    here.searchParams.append("cfg_id", e.target.id);
    here.searchParams.append("cfg_val", e.target.checked);

    window.location.href = here.href;
}

function update_timeout() {
    timeout = document.querySelector("#timeout_s").value;
    here.searchParams.append("timeout", timeout);
    window.location.href = here.href;
}

function toggle_power() {
    here.searchParams.append("cfg_id", "power_sw");
    window.location.href = here.href;
}

function update() {
    colorPicker = document.querySelector("#color-picker");
    const color = colorPicker.value

    const r = parseInt(color.substr(1, 2), 16)
    const g = parseInt(color.substr(3, 2), 16)
    const b = parseInt(color.substr(5, 2), 16)

    set_color(r, g, b);
}