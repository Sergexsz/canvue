<template>
  <div class="app">
    <h1>HONDA B-CAN</h1>

    <button class="btn" @click="openPort()"> open port</button>
    <button class="btn" @click="closePort()"> close port</button>
    <button class="btn" @click="listSerialPorts()"> list ports</button>
    <button class="btn" @click="writeMessage()"> submit test</button>

    <div id="error"></div>
    <!--    <div id="ports"></div>-->

    <table class="table">
      <thead style="font-weight: bold">
      <tr>
        <td>ID</td>
        <td>Кол-во</td>
        <td>Частота</td>
        <td>DLC</td>
        <td colspan="8" style="text-align: center">Данные</td>
        <td>Комментарий</td>
      </tr>
      </thead>
      <tbody>
      <template :key="device.id" v-for="device in deviceList">
        <tr>
          <!--          id-->
          <td>{{ device.id }}</td>
          <!--          dlc-->
          <td>{{ device.count }}</td>
          <td>± {{ device.frequency }}мс</td>
          <td>{{ device.dlc }}</td>

          <!--          data-->
          <dlc-item :key="n" v-for="n in 8" :data="device.data[n - 1]"></dlc-item>

          <!--          comment-->
          <td>
            <label>
              <textarea @blur="saveFile" style="height: 20px" class="form-control" v-model="device.comment"></textarea>
            </label>
          </td>

        </tr>
      </template>
      </tbody>
    </table>

    <hr>

    <write-table @write="writeToPort"></write-table>


  </div>
</template>

<script>
import moment from 'moment'
import DlcItem from "@/components/dlc-item";
import WriteTable from "@/components/write/write-table";

const serialport = require('serialport')
// const tableify = require('tableify')

export const sp = new serialport('/dev/tty.usbserial-14330', {
  baudRate: 115200,
});
//"/dev/tty.usbserial-141240"


export default {
  name: 'App',
  components: {WriteTable, DlcItem},
  data() {
    return {
      // device: '/dev/tty.usbserial-144220',
      // boudrate: 115200,
      deviceList: [],
    }
  },
  created() {
    this.listSerialPorts(); // once
    this.serialBegin();
    this.openFile();

  },
  methods: {
    addText(event) {
      this.handleMessage(this.parseEvent(event));
    },
    parseEvent(message) {
      if (message.indexOf('<--') > -1) {
        let device = message.slice(message.indexOf('-') + 2, message.indexOf('['));
        let m_length = message.slice(message.indexOf('[') + 1, message.indexOf(']'));
        let data = message.slice(message.indexOf(']') + 2, message.length).split('.');
        if (m_length > -1) {
          data = data.map(a => {
            return {
              value: a,
              time: moment.now()
            }
          })
          return {
            id: device,
            dlc: +m_length,
            count: 1,
            frequency: 0,
            data: data,
            message_at: moment.now(),
            comment: ''
          }
        }
      } else {
        console.log(message);
      }
      return null;
    },
    pushDevice(message) {
      this.deviceList.push(message);
    },
    ejectDevice() {

    },
    updateDevice(index, message) {
      let iDevice = this.deviceList[index];

      iDevice.dlc = message.dlc;

      // check if data changes
      let data = iDevice.data.map((item, index) => {
        if (item.value !== message.data[index].value) {
          return message.data[index]
        }
        return item;
      })
      iDevice.count++;
      iDevice.frequency = message.message_at - iDevice.message_at;
      iDevice.data = data;
      iDevice.message_at = message.message_at;
    },
    handleMessage(message) {
      if (!message) {
        return;
      }
      // check if device is only one
      let ids = this.deviceList.filter(device => {
        return device.id === message.id;
      });

      let devIndex = -1;
      // если одно
      if (ids.length < 2) {
        devIndex = (this.deviceList.map(device => {
          return device.id;
        }).indexOf(message.id));
      } else {
        // device more than 1
        // find by dlc length
        devIndex = this.deviceList.indexOf(ids[ids.map(id => {
              return id.dlc
            }).indexOf(message.dlc)]
        )
      }

      if (devIndex > -1) {
        /**
         * Устройство уже было на линии
         */
        this.updateDevice(devIndex, message);
      } else {
        /**
         * Устройство появилось на линии впервые
         */
        this.pushDevice(message);
      }
    },
    serialBegin() {
      const Readline = serialport.parsers.Readline;
      const parser = sp.pipe(new Readline({delimiter: '\r\n'}));
      parser.on('data', this.addText);
    },
    openPort() {
      sp.open(() => {

      });
    },
    closePort() {
      sp.close(() => {

      });
    },
    listSerialPorts() {
      serialport.list().then((ports, err) => {
        if (err) {
          document.getElementById('error').textContent = err.message
          return
        } else {
          document.getElementById('error').textContent = ''
        }
        console.log('ports', ports);

        if (ports.length === 0) {
          document.getElementById('error').textContent = 'No ports discovered'
        }

        // let tableHTML = tableify(ports)
        // document.getElementById('portss').innerHTML = tableHTML
      })
    },
    saveFile: function () {
      const data = JSON.stringify(this.deviceList.map(device => {
        return {
          id: device.id,
          dlc: device.dlc,
          comment: ''
        }
      }))
      const fs = require('fs');
      try {
        fs.writeFileSync('can-device.json', data, 'utf-8');
      } catch (e) {
        alert('Failed to save the file !');
      }
    },
    openFile: function () {
      // const fs = require('fs');
      // let jsonDB = fs.readFileSync('can-device.json', 'utf-8');
      // console.log(JSON.parse(jsonDB));
      // this.deviceList = JSON.parse(jsonDB);
    },
    writeToPort(message){
      sp.write(message);
    }
  },

}
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  text-align: center;
  margin-top: 60px;
}

table {
  width: 100%;
  border: 1px solid #eee;
  text-align: left;
}

table tr {
  transition: all .3s;
}

table tr:hover {
  background-color: #eee;
}

table td {
  padding: 1px 8px;
  border: 1px solid #eee;
}

input {
  width: 100%;
  border-color: #ccc;
  border-radius: 3px;
  height: auto;
  min-height: auto;
}

.form-control {
  width: 100%;
  border-color: #ccc;
  border-radius: 3px;
  height: auto;
  min-height: auto;
}
</style>
