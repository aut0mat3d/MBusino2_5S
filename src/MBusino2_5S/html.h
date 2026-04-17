const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino Setup</title>
    <style>
      *, ::after, ::before { box-sizing: border-box; }
      body { margin: 0; font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #2b3e40; color: #e0e0e0; }
      .container { max-width: 550px; margin: auto; padding: 15px; }
      h1 { text-align: center; color: #fff; margin-bottom: 5px; }
      h3 { margin-top: 0; color: #7db9b6; border-bottom: 1px solid #438287; padding-bottom: 5px; margin-bottom: 10px; }
      .hint { color: #ffb74d; font-size: 0.85rem; margin-top: 0; margin-bottom: 15px; }
      
      .tab-nav { display: flex; flex-wrap: wrap; background: #1a2627; border-radius: 5px; margin-bottom: 15px; }
      .tab-nav button { flex: 1; min-width: 80px; padding: 12px 5px; background: none; border: none; color: #888; cursor: pointer; font-weight: bold; font-size: 0.9rem; transition: 0.3s; }
      .tab-nav button:hover { color: #fff; }
      .tab-nav button.active { color: #fff; background: #438287; border-radius: 5px; }
      
      .tab-content { display: none; background: #304648; padding: 15px; border-radius: 5px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); margin-bottom: 20px;}
      .tab-content.active { display: block; }
      
      .form-group { margin-bottom: 12px; display: flex; flex-direction: column; }
      label { display: block; margin-bottom: 4px; font-size: 0.8rem; color: #bcd4d3; letter-spacing: 0.5px; }
      input, select { padding: 8px 10px; border: 1px solid #5a7b7c; background: #1a2627; color: #fff; border-radius: 4px; font-size: 0.95rem; align-self: stretch; min-width: 0; flex: 1; }
      input[type="checkbox"] { width: auto; align-self: auto; margin-right: 8px; cursor: pointer; }
      input:focus, select:focus { outline: none; border-color: #7db9b6; }
      
      .flex-row { display: flex; gap: 10px; align-items: flex-end; }
      .flex-row > div { flex: 1; }
      
      .slave-block { background: #2b3e40; padding: 10px; border-radius: 5px; margin-bottom: 15px; border-left: 3px solid #7db9b6; }
      .sensor-row { background: #2b3e40; padding: 10px; border-radius: 5px; margin-bottom: 10px; }
      .live-val { color: #81c784; font-weight: bold; float: right; text-transform: none; font-size: 0.9rem;}
      
      .action-bar { display: flex; gap: 10px; margin-top: 10px; }
      .btn-save { flex: 2; padding: 14px; background-color: #7db9b6; color: #1a2627; border: none; font-size: 1.1rem; font-weight: bold; border-radius: 5px; cursor: pointer; transition: 0.3s;}
      .btn-save:hover { background-color: #5a9b98; }
      .btn-reboot { flex: 1; padding: 14px; background-color: #e57373; color: #fff; border: none; font-size: 1.1rem; font-weight: bold; border-radius: 5px; cursor: pointer; transition: 0.3s;}
      .btn-reboot:hover { background-color: #ef5350; }
      
      .btn-small { background-color: #5a7b7c; color: #fff; padding: 9px 5px; text-decoration: none; border-radius: 4px; font-size: 0.85rem; display: block; text-align: center; border: none; cursor: pointer; font-weight: bold; transition: 0.3s;}
      .btn-small:hover { background-color: #7db9b6; color: #1a2627; }

      #save-banner { display: none; background-color: #e57373; color: #fff; text-align: center; padding: 12px; border-radius: 5px; margin-bottom: 15px; font-weight: 500; font-size: 0.95rem; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
      #save-banner a { color: #fff; text-decoration: underline; font-weight: bold; }
      
      #lock-banner { display: none; background-color: #ffb74d; color: #1a2627; text-align: center; padding: 12px; border-radius: 5px; margin-bottom: 15px; font-weight: bold; font-size: 0.95rem; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }

      /* --- MOBILE RESPONSIVE FIXES --- */
      @media (max-width: 600px) {
        .sensor-row.flex-row { flex-wrap: wrap; margin-bottom: 15px; }
        .sensor-row .form-group:nth-child(1) { flex: 1 1 auto !important; min-width: 250px; margin-bottom: 5px; }
        .sensor-row .form-group:nth-child(2) { flex: 0 0 100px !important; margin-left: 25px !important; }
      }
    </style>
    <script>
      let lastEditTime = 0;
      const LOCK_TIMEOUT_MS = 300000; 
      let availableProfiles = [];

      document.addEventListener("DOMContentLoaded", function() {
        const form = document.getElementById('config-form');
        
        // Helper-Function for ignoring inputs for not triggering "changes detected..."
        const shouldIgnore = (target) => {
          // Ignoriere alle File-Uploads
          if (target.type === 'file') return true;
          // Ignore Passwort-Checkbox and Checkboxes from Restore-Panel (all are starting with 'chk-')
          if (target.id && target.id.startsWith('chk-')) return true;
          return false;
        };

        form.addEventListener('input', (e) => {
          if (shouldIgnore(e.target)) return;
          lastEditTime = Date.now();
          updateLockBanner();
        });
        
        form.addEventListener('change', (e) => {
          if (shouldIgnore(e.target)) return;
          lastEditTime = Date.now();
          updateLockBanner();
        });

        setInterval(updateLockBanner, 1000);
        fetchProfiles();
      });

      function safeReboot(event) {
        if (lastEditTime > 0) {
          if (!confirm("Potentially unsaved changes, really reboot?")) {
            if (event) event.preventDefault();
            return;
          }
        }
        window.location.href = '/reboot';
      }

      function fetchProfiles() {
        fetch('/api/profiles')
          .then(res => res.json())
          .then(data => {
            availableProfiles = data;
            populateProfileDropdowns();
            renderProfileTable();
          }).catch(err => console.log("Failed to load profiles"));
      }

      function refreshSelectStyle(sel) {
        if (!sel || sel.selectedIndex === -1) return;
        let opt = sel.options[sel.selectedIndex];
        if (opt.text.includes("MISSING!")) {
          sel.style.border = "2px solid #e57373"; sel.style.color = "#e57373";
        } else if (opt.text.includes("\u26A0")) {
          sel.style.border = "2px solid #ffb74d"; sel.style.color = "#ffb74d";
        } else {
          sel.style.border = "1px solid #5a7b7c"; sel.style.color = "#fff";
        }
      }

      function handleProfileChange(sel) {
        sel.setAttribute('data-current', sel.value);
        refreshSelectStyle(sel);
        renderProfileTable(); // Update DEL buttons state
        lastEditTime = Date.now();
        updateLockBanner();
      }

      function populateProfileDropdowns() {
        for(let i=0; i<5; i++) {
          let sel = document.getElementById('prof-sel-'+i);
          if(!sel) continue;
          let currentVal = sel.getAttribute('data-current') || "";
          
          let html = '<option value="">-- Auto-Assign / Silent Mode --</option>';
          let found = false;

          availableProfiles.forEach(p => {
            let selected = (p.file === currentVal) ? 'selected' : '';
            if (p.file === currentVal) found = true;
            html += `<option value="${p.file}" ${selected}>${p.name} (${p.man})</option>`;
          });

          if (currentVal !== "" && !found) {
            html += `<option value="${currentVal}" selected style="color: #e57373; font-weight: bold;">\u26A0 ${currentVal} (MISSING!)</option>`;
          }
          sel.innerHTML = html;
          if (currentVal !== "") sel.value = currentVal;
          refreshSelectStyle(sel);
        }
      }

      function isProfileInUse(fname) {
        for(let i=0; i<5; i++) {
          let sel = document.getElementById('prof-sel-'+i);
          if(sel && sel.value === fname) return true;
        }
        return false;
      }

      function renderProfileTable() {
        let tbody = document.getElementById('profile-table-body');
        if(!tbody) return;
        tbody.innerHTML = '';
        availableProfiles.forEach(p => {
          let tr = document.createElement('tr');
          let typeLabel = p.type === 'system' ? '<span style="color:#ffb74d; font-size:0.8rem; white-space:nowrap;">[SYS]</span>' : '<span style="color:#81c784; font-size:0.8rem; white-space:nowrap;">[CUST]</span>';
          
          let delBtn = '';
          if (p.type !== 'system') {
            let inUse = isProfileInUse(p.file);
            let btnStyle = inUse ? 'background:#555; color:#888; cursor:not-allowed;' : 'background:#e57373;';
            let btnAction = inUse ? "alert('Profile used by Meter(s), could not delete!')" : `deleteProfile('${p.file}')`;
            delBtn = `<button type="button" class="btn-small" style="${btnStyle} display:inline-block; padding: 5px;" onclick="${btnAction}">DEL</button>`;
          }

          tr.innerHTML = `
            <td style="padding: 8px; border-bottom: 1px solid #438287; word-break: break-all;">${p.file} ${typeLabel}</td>
            <td style="padding: 8px; border-bottom: 1px solid #438287;">${p.man}</td>
            <td style="padding: 8px; border-bottom: 1px solid #438287; white-space: nowrap;">
               <a href="/profiles/${p.file}" download="${p.file}" class="btn-small" style="background:#5a7b7c; display:inline-block; padding: 5px; margin-right:5px;">GET</a>
               ${delBtn}
            </td>
          `;
          tbody.appendChild(tr);
        });
      }

      function uploadProfile() {
        const fileInput = document.getElementById('upload-profile-file');
        const file = fileInput.files[0];
        if (!file) return;
        if (!file.name.endsWith('.json')) { alert("Only .json files are allowed!"); fileInput.value = ''; return; }

        const formData = new FormData();
        formData.append("file", file, file.name);
        const btn = document.getElementById('btn-upload-trigger');
        const originalText = btn.innerText;
        btn.innerText = "Uploading..."; btn.disabled = true;

        fetch('/api/uploadProfile', { method: 'POST', body: formData })
        .then(res => {
          btn.innerText = originalText; btn.disabled = false;
          if (res.ok) { fetchProfiles(); } else { alert("Upload failed."); }
        }).catch(err => { btn.innerText = originalText; btn.disabled = false; alert("Connection error."); });
        fileInput.value = '';
      }

      function deleteProfile(filename) {
        if(confirm("Are you sure you want to delete " + filename + "?")) {
          fetch('/api/deleteProfile?file=' + encodeURIComponent(filename))
          .then(res => { if(res.ok) fetchProfiles(); else alert("Error."); });
        }
      }

      function updateLockBanner() {
        let banner = document.getElementById('lock-banner');
        if (lastEditTime === 0) { banner.style.display = 'none'; return; }
        let elapsed = Date.now() - lastEditTime;
        let remaining = Math.max(0, Math.floor((LOCK_TIMEOUT_MS - elapsed) / 1000));
        if (remaining > 0) {
          banner.style.display = 'block'; banner.innerText = "Values changed! Form sync locked: " + remaining + " s";
        } else { banner.style.display = 'none'; lastEditTime = 0; }
      }

      function openTab(tabId, btn) {
        document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
        document.querySelectorAll('.tab-nav button').forEach(el => el.classList.remove('active'));
        document.getElementById(tabId).classList.add('active');
        btn.classList.add('active');
      }

      function updateLive() {
        for(let i=0; i<5; i++) {
          let addrInput = document.getElementsByName('mbusAddress'+(i+1))[0];
          let nameInput = document.getElementsByName('slaveName'+(i+1))[0];
          let statusRow = document.getElementById('mb-status-row-'+i);
          if(addrInput && nameInput && statusRow) {
            let addr = parseInt(addrInput.value); let name = nameInput.value.trim();
            statusRow.style.display = (!isNaN(addr) && addr >= 0 && addr <= 250 && name !== "") ? 'flex' : 'none';
          }
        }
        fetch('/liveData').then(res => res.json()).then(data => {
          document.getElementById('sys-uptime').innerText = data.sys.uptime;
          document.getElementById('sys-heap').innerText = data.sys.heap;
          
          let isLocked = (lastEditTime > 0 && (Date.now() - lastEditTime) < LOCK_TIMEOUT_MS);
          if(!isLocked) {
            let mInt = document.getElementsByName('mbusInterval')[0]; let sInt = document.getElementsByName('sensorInterval')[0];
            if(mInt && document.activeElement !== mInt) mInt.value = data.sys.mInt;
            if(sInt && document.activeElement !== sInt) sInt.value = data.sys.sInt;
          }
          for(let i=0; i<7; i++) {
            let val = data.sensors[i]; let el = document.getElementById('ow-live-'+i);
            if(el) el.innerText = (val <= -120) ? "N/A" : val.toFixed(2) + " C";
            if(!isLocked) {
              let off = document.getElementsByName('offset'+(i+1))[0];
              if(off && document.activeElement !== off) off.value = data.offsets[i].toFixed(1);
            }
          }
          for(let i=0; i<5; i++) {
            let conn = document.getElementById('mb-conn-'+i); let man = document.getElementById('mb-man-'+i);
            let hex = document.getElementById('mb-hex-'+i); let fab = document.getElementById('mb-fab-'+i);
            let err = document.getElementById('mb-err-'+i);
            if(conn) {
              conn.innerText = data.mbus[i].conn ? "OK" : "ERR"; conn.style.color = data.mbus[i].conn ? "#81c784" : "#e57373";
              man.innerText = data.mbus[i].manAscii; hex.innerText = data.mbus[i].manHex;
              fab.innerText = data.mbus[i].fab ? "SN: " + data.mbus[i].fab : "";
              let errStr = data.mbus[i].err; err.innerText = (errStr === "0" || errStr === "") ? "" : "Error: " + errStr;
            }
            if(!isLocked) {
              let minF = document.getElementsByName('minFlow'+(i+1))[0]; let maxF = document.getElementsByName('maxFlow'+(i+1))[0];
              let maxP = document.getElementsByName('maxPower'+(i+1))[0]; let dead = document.getElementsByName('deadFlow'+(i+1))[0];
              if(minF && document.activeElement !== minF) minF.value = data.mbus[i].minF.toFixed(2);
              if(maxF && document.activeElement !== maxF) maxF.value = data.mbus[i].maxF.toFixed(2);
              if(maxP && document.activeElement !== maxP) maxP.value = data.mbus[i].maxP.toFixed(2);
              if(dead && document.activeElement !== dead) dead.value = data.mbus[i].dead.toFixed(3);
              
              let sel = document.getElementById('prof-sel-'+i);
              if (sel && sel.value === "") {
                let mAscii = data.mbus[i].manAscii;
                if (data.mbus[i].conn && mAscii && mAscii !== "GEN" && availableProfiles.length > 0) {
                  let matchCount = availableProfiles.filter(p => p.man === mAscii).length;
                  if (matchCount > 1) sel.options[0].text = "\u26A0 Multiple Profiles - Select Manually!";
                  else if (matchCount === 0) sel.options[0].text = "\u26A0 No Profile found for " + mAscii;
                  else sel.options[0].text = "-- Auto-Assign / Silent Mode --";
                } else { sel.options[0].text = "-- Auto-Assign / Silent Mode --"; }
                refreshSelectStyle(sel);
              }
            }
          }
        }).catch(err => {});
      }
      setInterval(updateLive, 3000);
      setTimeout(updateLive, 500);

      function getRaw(id) {
        fetch('/raw?id='+id).then(res => res.text()).then(text => {
          if(text === "NO_DATA_YET") alert("No data yet.");
          else prompt("RAW Telegram for Slave " + (id+1) + ":", text);
        });
      }

      function calAverage() {
        let sum = 0, count = 0, lives = [];
        for(let i=0; i<7; i++) {
          let valText = document.getElementById('ow-live-'+i).innerText;
          if(valText.includes("C")) {
            let v = parseFloat(valText); let off = parseFloat(document.getElementById('ow-off-'+i).value) || 0;
            sum += (v - off); count++; lives.push({idx: i, raw: (v - off)});
          }
        }
        if(count < 2) { alert("Need 2 sensors!"); return; }
        let avg = sum / count;
        lives.forEach(item => { document.getElementById('ow-off-'+item.idx).value = (avg - item.raw).toFixed(1); });
        lastEditTime = Date.now(); updateLockBanner();
      }

      function calTarget() {
        let target = parseFloat(document.getElementById('cal-target-val').value);
        if(isNaN(target)) return;
        for(let i=0; i<7; i++) {
          let valText = document.getElementById('ow-live-'+i).innerText;
          if(valText.includes("C")) {
            let v = parseFloat(valText); let off = parseFloat(document.getElementById('ow-off-'+i).value) || 0;
            document.getElementById('ow-off-'+i).value = (target - (v - off)).toFixed(1);
          }
        }
        lastEditTime = Date.now(); updateLockBanner();
      }

      function saveSettings(event) {
        event.preventDefault(); lastEditTime = 0; updateLockBanner();
        let params = new URLSearchParams(new FormData(document.getElementById('config-form'))).toString();
        fetch('/get?' + params).then(response => response.text()).then(text => {
          let banner = document.getElementById('save-banner'); banner.style.display = 'block';
          if(text === "AP") { banner.style.backgroundColor = "#e57373"; banner.innerHTML = "WLAN changed. <a href='/reboot'>Reboot</a>!"; }
          else if (text === "CHANGED") { banner.style.backgroundColor = "#7db9b6"; banner.innerHTML = "Settings saved! <a href='/reboot'>Reboot</a> recommended."; }
          else { banner.style.backgroundColor = "#5a7b7c"; banner.innerHTML = "No changes."; }
          window.scrollTo({ top: 0, behavior: 'smooth' });
        });
      }

      function safeReboot(event) {
        if (lastEditTime > 0) {
          if (!confirm("Potentially unsaved changes, really reboot?")) {
            if (event) event.preventDefault();
            return; // Cancel, no Reboot
          }
        }
        // No Changes, or User klicks OK:
        window.location.href = '/reboot';
      }
      
      let pwdsVisible = false;
      async function togglePasswords() {
        pwdsVisible = !pwdsVisible;
        let btn = document.getElementById('btn-show-pw');
        let pwFields = ['webPassword', 'otaPassword', 'password', 'password2', 'mqttPswrd'];

        if (pwdsVisible) {
          btn.innerText = "Hide Passwords";
          let nonce = Date.now().toString(); 
          let key = nonce + "%MBUSINO_NAME%" + "MBusinoSecretSalt"; 
          try {
            let res = await fetch('/getPasswords?nonce=' + nonce);
            if(res.ok) {
              let data = await res.json();
              const deobfuscate = (hexStr, k) => {
                if(!hexStr) return ""; let out = "";
                for(let i=0; i < hexStr.length; i+=2) {
                  let charCode = parseInt(hexStr.substr(i, 2), 16);
                  let modIndex = (i/2) - k.length * Math.floor((i/2) / k.length);
                  out += String.fromCharCode(charCode ^ k.charCodeAt(modIndex));
                } return out;
              };
              let map = {webPassword:'wp', otaPassword:'op', password:'p1', password2:'p2', mqttPswrd:'mp'};
              pwFields.forEach(f => {
                let els = document.getElementsByName(f);
                if(els.length > 0) { 
                  let dec = deobfuscate(data[map[f]], key); if(dec !== "") els[0].value = dec; els[0].type = 'text'; 
                }
              });
            }
          } catch(e) {}
        } else { 
          btn.innerText = "Show Passwords";
          pwFields.forEach(f => { let els = document.getElementsByName(f); if(els.length > 0) els[0].type = 'password'; }); 
        }
      }

      let stagedConfig = null;
      function handleFileSelect(event) {
        let file = event.target.files[0]; if (!file) return;
        let reader = new FileReader();
        reader.onload = function(e) {
          try {
            stagedConfig = JSON.parse(e.target.result);
            document.getElementById('restore-panel').style.display = 'block';
          } catch (err) { alert("JSON Error."); }
        }; reader.readAsText(file);
      }

      function applyConfigToForm() {
        if(!stagedConfig) return;
        const setVal = (name, val) => { let els = document.getElementsByName(name); if(els.length > 0 && val !== undefined) els[0].value = val; };
        if(document.getElementById('chk-sys').checked && stagedConfig.system) {
          setVal('name', stagedConfig.system.deviceName);
          setVal('webPassword', stagedConfig.system.webPassword);
          setVal('otaPassword', stagedConfig.system.otaPassword);
          setVal('haAd', stagedConfig.system.haAutodisc ? '1' : '0');
          setVal('telegramDebug', stagedConfig.system.telegramDebug ? '1' : '0');
        }

        if(document.getElementById('chk-net').checked && stagedConfig.network) {
          if(stagedConfig.network.wifi) {
            setVal('ssid', stagedConfig.network.wifi.ssid1);
            setVal('password', stagedConfig.network.wifi.pwd1);
            setVal('ssid2', stagedConfig.network.wifi.ssid2);
            setVal('password2', stagedConfig.network.wifi.pwd2);
            setVal('apChannel', stagedConfig.network.wifi.apChannel);
          }
          // --- FIX: ESP-NOW Restore ---
          if(stagedConfig.network.espnow) {
            setVal('espNowEnable', stagedConfig.network.espnow.enable ? '1' : '0');
            setVal('espNowMac', stagedConfig.network.espnow.mac);
          }
        }

        if(document.getElementById('chk-mqtt').checked && stagedConfig.network && stagedConfig.network.mqtt) {
          setVal('broker', stagedConfig.network.mqtt.broker);
          setVal('mqttPort', stagedConfig.network.mqtt.port);
          setVal('mqttUser', stagedConfig.network.mqtt.user);
          setVal('mqttPswrd', stagedConfig.network.mqtt.pwd);
        }

        if(document.getElementById('chk-mbus').checked && stagedConfig.mbus) {
          setVal('mbusSlaves', stagedConfig.mbus.activeSlaves);
          setVal('mbusInterval', stagedConfig.mbus.pollInterval_ms / 1000); 
          if(stagedConfig.mbus.slaves) {
            stagedConfig.mbus.slaves.forEach((slave, i) => {
              setVal('mbusAddress'+(i+1), slave.address); setVal('slaveName'+(i+1), slave.name);
              setVal('minFlow'+(i+1), slave.minFlow); setVal('maxFlow'+(i+1), slave.maxFlow);
              setVal('maxPower'+(i+1), slave.maxPower); setVal('deadFlow'+(i+1), slave.deadband);
              let sel = document.getElementsByName('profile'+(i+1))[0];
              if (sel && slave.profile !== undefined) sel.setAttribute('data-current', slave.profile);
            });
            populateProfileDropdowns();
          }
        }
        
        if(document.getElementById('chk-sens').checked && stagedConfig.sensors) {
          setVal('owSensors', stagedConfig.sensors.activeOnewire);
          setVal('sensorInterval', stagedConfig.sensors.pollInterval_ms / 1000); 
          setVal('i2cMode', stagedConfig.sensors.i2cMode);
          setVal('bmeName', stagedConfig.sensors.bmeName);
          
          if(stagedConfig.sensors.onewire) {
            stagedConfig.sensors.onewire.forEach((sens, i) => {
              setVal('sensorName'+(i+1), sens.name);
              setVal('offset'+(i+1), sens.offset);
            });
          }
        }
        document.getElementById('restore-panel').style.display = 'none';
        lastEditTime = Date.now(); updateLockBanner();
      }
    </script>
  </head>
  <body>
    <div class='container'>
      <h1><i>%MBUSINO_NAME%</i></h1>
      <div style="text-align: center; font-size: 0.75rem; color: #5a7b7c; margin-bottom: 10px; margin-top: -5px;">
        rev.: <span style="color: #7db9b6; font-weight: bold;">%MBUSINO_VERSION%</span> |
        UPTIME: <span id="sys-uptime" style="color: #7db9b6; font-weight: bold;">...</span> | 
        FREE HEAP: <span id="sys-heap" style="color: #7db9b6; font-weight: bold;">...</span> KB
      </div>
      <div id="save-banner"></div>
      <div id="lock-banner"></div>
      <div class="tab-nav">
        <button type="button" class="active" onclick="openTab('tab-gen', this)">General</button>
        <button type="button" onclick="openTab('tab-net', this)">Network</button>
        <button type="button" onclick="openTab('tab-mbus', this)">M-Bus</button>
        <button type="button" onclick="openTab('tab-sens', this)">Sensors</button>
      </div>
      <form id="config-form" onsubmit="saveSettings(event)">
        <div id="tab-gen" class="tab-content active">
          <h3>System Settings</h3>
          <div class='form-group'><label>Device Name (Network Hostname)</label><input type='text' value='%MBUSINO_NAME%' maxlength='30' name='name'></div>
          <div class='form-group'><label>Web-UI Password (NONE: disabled)</label><input type='%PWD_TYPE%' value='%WEB_PWD%' maxlength='64' name='webPassword'></div>
          <div class='flex-row'>
             <div class='form-group' style="flex: 2;"><label>OTA Update Password</label><input type='%PWD_TYPE%' value='%OTA_PWD%' maxlength='64' name='otaPassword'></div>
             <div class='form-group' style="flex: 1;"><a href="/update" class="btn-small">OTA Update</a></div>
          </div>
          <div class='flex-row'>
            <div class='form-group'><label>HA Auto-Discovery</label>
              <select name='haAd'><option value='0' %HA_SEL_0%>No</option><option value='1' %HA_SEL_1%>Yes</option></select>
            </div>
            <div class='form-group'><label>Telegram Debug</label>
              <select name='telegramDebug'><option value='0' %DBG_SEL_0%>No</option><option value='1' %DBG_SEL_1%>Yes</option></select>
            </div>
          </div>
          <h3 style="margin-top: 20px;">Backup & Restore</h3>
          
          <input type="file" id="configFile" style="display:none" accept=".json" onchange="handleFileSelect(event)">
          
          <div class='flex-row'>
            <div class='form-group' style="flex: 1;"><a href="/backup.json" class="btn-small" style="background-color: #438287; padding: 12px; display: flex; align-items: center; justify-content: center;">Download Config Backup</a></div>
            <div class='form-group' style="flex: 1; display: flex;"><button type="button" class="btn-small" style="padding: 12px; flex: 1;" onclick="document.getElementById('configFile').click()">Upload Config Backup</button></div>
          </div>
          
          <div id="restore-panel" style="display:none; background: #1a2627; padding: 15px; border-radius: 5px; margin-top: 15px; border-left: 3px solid #ffb74d;">
            <div style="margin-bottom: 10px; color: #ffb74d; font-weight: bold; font-size: 0.9rem;" id="restore-meta"></div>
            <div class='flex-row' style="flex-wrap: wrap; gap: 15px; margin-bottom: 15px;">
              <label><input type="checkbox" id="chk-sys" checked> System (Name, PWs)</label>
              <label><input type="checkbox" id="chk-net"> WLAN Network</label>
              <label><input type="checkbox" id="chk-mqtt" checked> MQTT</label>
              <label><input type="checkbox" id="chk-mbus" checked> M-Bus</label>
              <label><input type="checkbox" id="chk-sens" checked> Sensors</label>
            </div>
            <button type="button" class="btn-small" style="background-color: #ffb74d; color: #1a2627;" onclick="applyConfigToForm()">Inject into Form</button>
          </div>
        </div>

        <div id="tab-net" class="tab-content">
          <h3>WLAN Settings</h3>
          <div class='form-group'><label>Primary SSID</label><input type='text' value='%SSID1%' maxlength='64' name='ssid'></div>
          <div class='form-group'><label>Primary Password</label><input type='%PWD_TYPE%' value='%PWD1%' maxlength='64' name='password'></div>
          <div class='form-group'><label>Fallback SSID (optional)</label><input type='text' value='%SSID2%' maxlength='64' name='ssid2'></div>
          <div class='form-group'><label>Fallback Password</label><input type='%PWD_TYPE%' value='%PWD2%' maxlength='64' name='password2'></div>
          <div class='form-group'><label>AP Channel</label><input type='number' min='1' max='13' value='%AP_CHAN%' name='apChannel'></div>
          
          <h3 style="margin-top: 20px;">MQTT Broker</h3>
          <div class='flex-row'>
            <div class='form-group' style="flex: 1;"><label>Broker IP/Host</label><input type='text' value='%BROKER%' maxlength='64' name='broker'></div>
            <div class='form-group' style="flex: 0 0 100px;"><label>Port</label><input type='number' value='%PORT%' name='mqttPort'></div>
          </div>
          <div class='form-group'><label>MQTT User (optional)</label><input type='text' value='%MQTT_USER%' maxlength='64' name='mqttUser'></div>
          <div class='form-group'><label>MQTT Password (optional)</label><input type='%PWD_TYPE%' value='%MQTT_PWD%' maxlength='64' name='mqttPswrd'></div>
          
          <h3 style="margin-top: 20px;">ESP-NOW Broadcast</h3>
          <p class="hint">Send sensor data directly to other ESP-NOW capable Devices.</p>
          
          <div style="background: #1a2627; padding: 10px; border-radius: 5px; margin-bottom: 15px; border-left: 3px solid #7db9b6;">
            <div style="font-size: 0.85rem; color: #bcd4d3;">Your Unique ESP-NOW System ID (based on Device Name):</div>
            <div style="font-family: monospace; font-size: 1.1rem; color: #7db9b6; font-weight: bold; margin-top: 3px;">%ESPNOW_SYSID%</div>
          </div>

          <div class='flex-row'>
            <div class='form-group' style="flex: 1;"><label>Enable ESP-NOW</label>
              <select name='espNowEnable'><option value='0' %ESPNOW_EN_0%>Disabled</option><option value='1' %ESPNOW_EN_1%>Enabled</option></select>
            </div>
            <div class='form-group' style="flex: 2;"><label>Target MAC</label>
              <input type='text' value='%ESPNOW_MAC%' maxlength='17' name='espNowMac' placeholder='FF:FF:FF:FF:FF:FF' style="max-width: 180px;">
            </div>
          </div>
          </div>

        <div id="tab-mbus" class="tab-content">
          <h3>M-Bus Configuration</h3>
          <p class="hint">Note: Slaves are only populated to Home Assistant if they have a given name. Limits > 990 will be ignored.</p>
          
          <div class='flex-row' style="margin-bottom: 15px;">
            <div class='form-group' style="flex: 1;"><label>Number of Slaves</label><input type='number' value='%MBUS_SLAVES%' min='0' max='5' name='mbusSlaves'></div>
            <div class='form-group' style="flex: 1;"><label>Poll Interval (s)</label><input type='number' value='%MBUS_INT%' name='mbusInterval'></div>
            <div class='form-group' style="flex: 0 0 80px;"><label>&nbsp;</label><a href="/setaddress" class="btn-small" style="padding: 9px 5px;">Set Addr</a></div>
          </div>
          
          <div class='slave-block'>
            <div class='flex-row'><div class='form-group'><label>Addr 1</label><input type='number' value='%ADDR1%' name='mbusAddress1'></div><div class='form-group' style="flex: 3;"><label>Name 1</label><input type='text' value='%MNAME1%' maxlength='30' name='slaveName1'></div></div>
            <div class='form-group'><label>Assigned JSON Profile</label><select name='profile1' id='prof-sel-0' data-current='%PROF1%' onchange='handleProfileChange(this)'></select></div>
            <div class='flex-row'>
              <div class='form-group'><label>Max Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MAXF1%' name='maxFlow1'></div>
              <div class='form-group'><label>Max Power (kW)</label><input type='number' step='0.01' value='%MAXP1%' name='maxPower1'></div>
            </div>
            <div class='flex-row'>
              <div class='form-group'><label>Min Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MINF1%' name='minFlow1'></div>
              <div class='form-group'><label>Deadband (m&sup3;/h)</label><input type='number' step='0.001' value='%DEAD1%' name='deadFlow1'></div>
            </div>
            <div class='flex-row' id='mb-status-row-0' style="margin-top: 5px; font-size: 0.85rem; color: #bcd4d3; display: none; align-items: center; border-top: 1px solid #438287; padding-top: 8px;">
              <div style="flex: 3; line-height: 1.4;">
                <div>ManID: <strong id="mb-man-0" style="color:#fff;">...</strong> (<span id="mb-hex-0"></span>)</div>
                <div><span id="mb-fab-0" style="color:#7db9b6; font-weight:bold;"></span></div>
                <div>Status: <strong id="mb-conn-0">...</strong> <span id="mb-err-0" style="color:#e57373; margin-left:5px;"></span></div>
              </div>
              <div style="flex: 1; text-align: right;">
                <button type="button" class="btn-small" onclick="getRaw(0)" style="display: inline-block; padding: 8px 15px;">Get RAW</button>
              </div>
            </div>
          </div>

          <div class='slave-block'>
            <div class='flex-row'><div class='form-group'><label>Addr 2</label><input type='number' value='%ADDR2%' name='mbusAddress2'></div><div class='form-group' style="flex: 3;"><label>Name 2</label><input type='text' value='%MNAME2%' maxlength='30' name='slaveName2'></div></div>
            <div class='form-group'><label>Assigned JSON Profile</label><select name='profile2' id='prof-sel-1' data-current='%PROF2%' onchange='handleProfileChange(this)'></select></div>
            <div class='flex-row'>
              <div class='form-group'><label>Max Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MAXF2%' name='maxFlow2'></div>
              <div class='form-group'><label>Max Power (kW)</label><input type='number' step='0.01' value='%MAXP2%' name='maxPower2'></div>
            </div>
            <div class='flex-row'>
              <div class='form-group'><label>Min Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MINF2%' name='minFlow2'></div>
              <div class='form-group'><label>Deadband (m&sup3;/h)</label><input type='number' step='0.001' value='%DEAD2%' name='deadFlow2'></div>
            </div>
            <div class='flex-row' id='mb-status-row-1' style="margin-top: 5px; font-size: 0.85rem; color: #bcd4d3; display: none; align-items: center; border-top: 1px solid #438287; padding-top: 8px;">
              <div style="flex: 3; line-height: 1.4;">
                <div>ManID: <strong id="mb-man-1" style="color:#fff;">...</strong> (<span id="mb-hex-1"></span>)</div>
                <div><span id="mb-fab-1" style="color:#7db9b6; font-weight:bold;"></span></div>
                <div>Status: <strong id="mb-conn-1">...</strong> <span id="mb-err-1" style="color:#e57373; margin-left:5px;"></span></div>
              </div>
              <div style="flex: 1; text-align: right;">
                <button type="button" class="btn-small" onclick="getRaw(1)" style="display: inline-block; padding: 8px 15px;">Get RAW</button>
              </div>
            </div>
          </div>

          <div class='slave-block'>
            <div class='flex-row'><div class='form-group'><label>Addr 3</label><input type='number' value='%ADDR3%' name='mbusAddress3'></div><div class='form-group' style="flex: 3;"><label>Name 3</label><input type='text' value='%MNAME3%' maxlength='30' name='slaveName3'></div></div>
            <div class='form-group'><label>Assigned JSON Profile</label><select name='profile3' id='prof-sel-2' data-current='%PROF3%' onchange='handleProfileChange(this)'></select></div>
            <div class='flex-row'>
              <div class='form-group'><label>Max Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MAXF3%' name='maxFlow3'></div>
              <div class='form-group'><label>Max Power (kW)</label><input type='number' step='0.01' value='%MAXP3%' name='maxPower3'></div>
            </div>
            <div class='flex-row'>
              <div class='form-group'><label>Min Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MINF3%' name='minFlow3'></div>
              <div class='form-group'><label>Deadband (m&sup3;/h)</label><input type='number' step='0.001' value='%DEAD3%' name='deadFlow3'></div>
            </div>
            <div class='flex-row' id='mb-status-row-2' style="margin-top: 5px; font-size: 0.85rem; color: #bcd4d3; display: none; align-items: center; border-top: 1px solid #438287; padding-top: 8px;">
              <div style="flex: 3; line-height: 1.4;">
                <div>ManID: <strong id="mb-man-2" style="color:#fff;">...</strong> (<span id="mb-hex-2"></span>)</div>
                <div><span id="mb-fab-2" style="color:#7db9b6; font-weight:bold;"></span></div>
                <div>Status: <strong id="mb-conn-2">...</strong> <span id="mb-err-2" style="color:#e57373; margin-left:5px;"></span></div>
              </div>
              <div style="flex: 1; text-align: right;">
                <button type="button" class="btn-small" onclick="getRaw(2)" style="display: inline-block; padding: 8px 15px;">Get RAW</button>
              </div>
            </div>
          </div>

          <div class='slave-block'>
            <div class='flex-row'><div class='form-group'><label>Addr 4</label><input type='number' value='%ADDR4%' name='mbusAddress4'></div><div class='form-group' style="flex: 3;"><label>Name 4</label><input type='text' value='%MNAME4%' maxlength='30' name='slaveName4'></div></div>
            <div class='form-group'><label>Assigned JSON Profile</label><select name='profile4' id='prof-sel-3' data-current='%PROF4%' onchange='handleProfileChange(this)'></select></div>
            <div class='flex-row'>
              <div class='form-group'><label>Max Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MAXF4%' name='maxFlow4'></div>
              <div class='form-group'><label>Max Power (kW)</label><input type='number' step='0.01' value='%MAXP4%' name='maxPower4'></div>
            </div>
            <div class='flex-row'>
              <div class='form-group'><label>Min Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MINF4%' name='minFlow4'></div>
              <div class='form-group'><label>Deadband (m&sup3;/h)</label><input type='number' step='0.001' value='%DEAD4%' name='deadFlow4'></div>
            </div>
            <div class='flex-row' id='mb-status-row-3' style="margin-top: 5px; font-size: 0.85rem; color: #bcd4d3; display: none; align-items: center; border-top: 1px solid #438287; padding-top: 8px;">
              <div style="flex: 3; line-height: 1.4;">
                <div>ManID: <strong id="mb-man-3" style="color:#fff;">...</strong> (<span id="mb-hex-3"></span>)</div>
                <div><span id="mb-fab-3" style="color:#7db9b6; font-weight:bold;"></span></div>
                <div>Status: <strong id="mb-conn-3">...</strong> <span id="mb-err-3" style="color:#e57373; margin-left:5px;"></span></div>
              </div>
              <div style="flex: 1; text-align: right;">
                <button type="button" class="btn-small" onclick="getRaw(3)" style="display: inline-block; padding: 8px 15px;">Get RAW</button>
              </div>
            </div>
          </div>

          <div class='slave-block'>
            <div class='flex-row'><div class='form-group'><label>Addr 5</label><input type='number' value='%ADDR5%' name='mbusAddress5'></div><div class='form-group' style="flex: 3;"><label>Name 5</label><input type='text' value='%MNAME5%' maxlength='30' name='slaveName5'></div></div>
            <div class='form-group'><label>Assigned JSON Profile</label><select name='profile5' id='prof-sel-4' data-current='%PROF5%' onchange='handleProfileChange(this)'></select></div>
            <div class='flex-row'>
              <div class='form-group'><label>Max Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MAXF5%' name='maxFlow5'></div>
              <div class='form-group'><label>Max Power (kW)</label><input type='number' step='0.01' value='%MAXP5%' name='maxPower5'></div>
            </div>
            <div class='flex-row'>
              <div class='form-group'><label>Min Flow (m&sup3;/h)</label><input type='number' step='0.01' value='%MINF5%' name='minFlow5'></div>
              <div class='form-group'><label>Deadband (m&sup3;/h)</label><input type='number' step='0.001' value='%DEAD5%' name='deadFlow5'></div>
            </div>
            <div class='flex-row' id='mb-status-row-4' style="margin-top: 5px; font-size: 0.85rem; color: #bcd4d3; display: none; align-items: center; border-top: 1px solid #438287; padding-top: 8px;">
              <div style="flex: 3; line-height: 1.4;">
                <div>ManID: <strong id="mb-man-4" style="color:#fff;">...</strong> (<span id="mb-hex-4"></span>)</div>
                <div><span id="mb-fab-4" style="color:#7db9b6; font-weight:bold;"></span></div>
                <div>Status: <strong id="mb-conn-4">...</strong> <span id="mb-err-4" style="color:#e57373; margin-left:5px;"></span></div>
              </div>
              <div style="flex: 1; text-align: right;">
                <button type="button" class="btn-small" onclick="getRaw(4)" style="display: inline-block; padding: 8px 15px;">Get RAW</button>
              </div>
            </div>
          </div>

          <div style="margin-top: 30px; border-top: 2px solid #7db9b6; padding-top: 15px;">
            <h3>Profile Manager</h3>
            <p class="hint">Upload custom .json profiles or download system templates to edit them.</p>
            <div style="overflow-x: auto; display: flex;">
              <table style="flex: 1; text-align: left; border-collapse: collapse; margin-bottom: 15px; font-size: 0.85rem;">
                <thead>
                  <tr style="border-bottom: 1px solid #5a7b7c; color: #bcd4d3;">
                    <th style="padding: 8px;">File</th>
                    <th style="padding: 8px;">ManID</th>
                    <th style="padding: 8px;">Action</th>
                  </tr>
                </thead>
                <tbody id="profile-table-body">
                </tbody>
              </table>
            </div>
            
            <input type="file" id="upload-profile-file" accept=".json" style="display:none" onchange="uploadProfile()">
            
            <div style="display: flex;">
              <button type="button" id="btn-upload-trigger" class="btn-small" style="flex: 1; background:#438287; padding: 12px; margin-top:5px;" onclick="document.getElementById('upload-profile-file').click()">Upload Custom Profile</button>
            </div>
          </div>

        </div> 
        
        <div id="tab-sens" class="tab-content">
          <h3>OneWire (DS18B20)</h3>
          <p class="hint">Note: Sensors are only populated to Home Assistant if they have a given name.</p>
          
          <div class='flex-row' style="margin-bottom: 15px;">
            <div class='form-group'><label>Number of Sensors</label><input type='number' value='%SENS_NUM%' min='0' max='7' name='owSensors'></div>
            <div class='form-group'><label>Poll Interval (sec)</label><input type='number' value='%SENS_INT%' name='sensorInterval'></div>
          </div>
          
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 1 Name <span class="live-val">Live: <span id="ow-live-0">--.--</span></span></label><input type='text' value='%SNAME1%' placeholder='Name Sensor 1' maxlength='30' name='sensorName1'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF1%' name='offset1' id="ow-off-0"></div></div>
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 2 Name <span class="live-val">Live: <span id="ow-live-1">--.--</span></span></label><input type='text' value='%SNAME2%' placeholder='Name Sensor 2' maxlength='30' name='sensorName2'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF2%' name='offset2' id="ow-off-1"></div></div>
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 3 Name <span class="live-val">Live: <span id="ow-live-2">--.--</span></span></label><input type='text' value='%SNAME3%' placeholder='Name Sensor 3' maxlength='30' name='sensorName3'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF3%' name='offset3' id="ow-off-2"></div></div>
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 4 Name <span class="live-val">Live: <span id="ow-live-3">--.--</span></span></label><input type='text' value='%SNAME4%' placeholder='Name Sensor 4' maxlength='30' name='sensorName4'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF4%' name='offset4' id="ow-off-3"></div></div>
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 5 Name <span class="live-val">Live: <span id="ow-live-4">--.--</span></span></label><input type='text' value='%SNAME5%' placeholder='Name Sensor 5' maxlength='30' name='sensorName5'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF5%' name='offset5' id="ow-off-4"></div></div>
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 6 (No I2C) <span class="live-val">Live: <span id="ow-live-5">--.--</span></span></label><input type='text' value='%SNAME6%' placeholder='Name Sensor 6' maxlength='30' name='sensorName6'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF6%' name='offset6' id="ow-off-5"></div></div>
          <div class='sensor-row flex-row'><div class='form-group' style="flex: 2.5;"><label>Sensor 7 (No I2C) <span class="live-val">Live: <span id="ow-live-6">--.--</span></span></label><input type='text' value='%SNAME7%' placeholder='Name Sensor 7' maxlength='30' name='sensorName7'></div><div class='form-group' style="flex: 1;"><label>Offset</label><input type='number' step='0.1' value='%SOFF7%' name='offset7' id="ow-off-6"></div></div>
          
          <h4 style="margin-top: 20px; margin-bottom: 5px; color: #7db9b6; border-bottom: 1px solid #438287; padding-bottom: 5px;">Smart Calibration</h4>
          
          <div style="display: grid; grid-template-columns: 140px 140px; gap: 10px; margin-top: 15px; margin-bottom: 30px;">
            <input type="number" step="0.1" id="cal-target-val" placeholder="Target °C" style="margin: 0; padding: 9px 10px;">
            <button type="button" class="btn-small" style="padding: 10px; margin: 0; display: flex; align-items: center; justify-content: center;" onclick="calTarget()">Cal to Target</button>
            
            <div></div> <button type="button" class="btn-small" style="padding: 10px; margin: 0; display: flex; align-items: center; justify-content: center;" onclick="calAverage()">Cal to Average</button>
          </div>
          
          <h3 style="margin-top: 30px;">I2C Interface</h3>
          <div class='form-group'><label>I2C Port Usage</label>
            <select name='i2cMode'>
              <option value='0' %I2C_SEL_0%>Disabled</option>
              <option value='1' %I2C_SEL_1%>BME280 (0x76 - Default)</option>
              <option value='2' %I2C_SEL_2%>BME280 (0x77 - Alternate)</option>
            </select>
          </div>
          <div class='form-group'><label>BME280 Name</label><input type='text' value='%BME_NAME%' maxlength='30' name='bmeName'></div>
        </div>

        <div style="margin-top: 20px; margin-bottom: 15px; text-align: center;">
          <button type="button" id="btn-show-pw" class="btn-small" style="display: inline-block; padding: 8px 20px; border: 1px solid #5a7b7c;" onclick="togglePasswords()">Show Passwords</button>
        </div>
        
        <div class="action-bar">
          <button type='submit' class="btn-save">Save Settings</button>
          <button type='button' class="btn-reboot" onclick="safeReboot(event)">Reboot</button>
        </div>
        
        <div style="text-align: center; margin-top: 25px;">
          <a href='%REPO_URL%' target='_blank' style="color: #5a7b7c; text-decoration: none; font-size: 0.85rem; font-weight: bold;">Project Home (GitHub)</a>
        </div>
      </form>
    </div>
  </body>
</html>
)rawliteral";

const char setAddress_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino Setup</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      h1 {
        text-align: center
      }

      .btn {
        display: inline-block;
        font-weight: 400;
        line-height: 1.5;
        color: #212529;
        text-align: center;
        text-decoration: none;
        vertical-align: middle;
        cursor: pointer;
        -webkit-user-select: none;
        -moz-user-select: none;
        user-select: none;
        background-color: transparent;
        border: 1px solid transparent;
        padding: .375rem .75rem;
        font-size: 1rem;
        border-radius: .25rem;
        transition: color .15s ease-in-out, background-color .15s ease-in-out, border-color .15s ease-in-out, box-shadow .15s ease-in-out
      }

      .btn-primary {
        color: #fff;
        background-color: #0d6efd;
        border-color: #0d6efd
      }
    </style>
  </head>

  <body>
    <main class='form-signin'>
      <h1 class=''><i>MBusino</i></h1><br>
      <h3 style='text-align:center'>Set Address for one connected slave</h3><br>
      <form style='text-align:center' action='/get'>
        Address(1-250): <input type='number' name='newAddress'>
        <input type='submit' value='Set'>
      </form><br/>
      <p style='text-align:center'><a href='/' style='color:#3F4CFB'>home</a></p>
    </main>
  </body>
</html>
)rawliteral";

const char update_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino update</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      h1 {
        text-align: center
      }
    </style>
  </head>
  <body>
    <main class='form-signin'>
         <h1 class=''><i>MBusino</i> update</h1><br>
          <p style='text-align:center'> select a xxx.bin file </p><br>
          <form style='text-align:center' method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'>
          </form><br/>
          <p style='text-align:center'> MBusino will restart after update </p><br>
          <p style='text-align:center'><a href='/' style='color:#3F4CFB'>home</a></p>
    </main>
  </body>
</html>)rawliteral";

const size_t update_htmlLength = strlen_P(update_html);
