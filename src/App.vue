<template>
  <div class="app">
    <div class="container">
      <el-select v-model="selectedPort"
                 class="el-select--small"
                 @change="createSerial">
        <el-option v-for="port in ports"
                   :key="port.path"
                   :value="port.path"
                   :label="port.path"
        ></el-option>

        <template #prefix>
          <button class="btn btn-sm" @click="listSerialPorts">re</button>
          <el-tag round size="small" :type="portClosed ? 'danger': 'success'"> {{ portClosed ? 'Закрыт' : 'Открыт' }}</el-tag>
        </template>
      </el-select>
      <div class="d-flex">
        <button class="btn btn-success" v-if="device && portClosed" @click="openPort()">Открыть порт</button>
        <button class="btn btn-danger" v-if="device && !portClosed" @click="closePort()">Закрыть порт</button>
      </div>

      <button class="btn" @click="dlcList = []"> удалить устройства</button>

      <div id="error"></div>

      <inc-table v-if="dlcList && dlcList.length > 0" :dlcList="dlcList"></inc-table>

      <write-table @write="writeToPort"></write-table>

    </div>
  </div>
</template>

<script>
// import _ from 'lodash'
import moment from 'moment'
import WriteTable from "@/components/write/write-table";
import IncTable from "@/components/incoming/inc-table";
import {ElMessage} from "element-plus";

const fs = require('fs');
const {SerialPort, ReadlineParser} = require('serialport')

export default {
  name: 'App',
  components: {IncTable, WriteTable},
  data() {
    return {
      device: null, // рабочее устройство
      baudRate: 115200, // скорость порта
      selectedPort: null, // порт из списка доступных
      ports: [], // все найденные порты
      dlcList: [], // список устройств
      portClosed: true, // статус порта
    }
  },
  async mounted() {
    // авто загрузка портов
    await this.listSerialPorts();
    this.openFile();
  },
  methods: {

    // загрузить список портов
    listSerialPorts() {
      SerialPort.list().then((ports, err) => {
        if (err != null || err == undefined) {
          let devices = [];
          for (const device of ports) {
            if (device.vendorId != undefined) {
              devices.push(device);
            }
          }
          if (devices.length == 0) {
            this.noDevices = true;
          } else {
            this.noDevices = false;
          }
          this.ports = devices;
          // console.log('ports: ', ports);
        }
      })
    },

    // создатель считывателя
    createSerial() {
      this.closePort(); // close before change
      if (this.selectedPort) {
        this.device = new SerialPort({path: this.selectedPort, baudRate: this.baudRate}, () => {
          this.openPort();
        });
      }
    },
    // открыть порт
    openPort() {
      if (this.device)
        this.device.open(() => {
          ElMessage.success('Порт открыт')
          // console.log('port opened');
          this.portClosed = false;
          this.serialBegin(); // начать считывать
        })
    },
    closePort() {
      if (this.device)
        this.device.close(() => {
          this.portClosed = true;
          ElMessage.warning('Порт закрыт')
          // console.log('port closed');
        });
    },
    serialBegin() {
      if (this.device) {
        // console.log('serialBegin');
        const parser = new ReadlineParser({delimiter: '\r\n'});
        this.device.pipe(parser)
        parser.on('data', this.addText);
        ElMessage.success('Идет обмен данными')
      }
    },
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
    // добавить устройство в список если его еще нет
    pushDevice(message) {
      this.dlcList.push({
        ...message,
        count: 1,
        data: []
      });
    },

    //убрать из списка
    ejectDevice() {

    },
    // обновить последнее сообщение устройства
    updateDevice(index, message) {
      // console.log(message.data);
      let iDevice = this.dlcList[index];
      iDevice.dlc = message.dlc;
      //
      // check if data changes
      // let data = _.map(iDevice.data, (item, index) => {
      //   console.log(item, index)
      //   if (item.value !== message.data[index].value) {
      //     return message.data[index]
      //   }
      //   return item;
      // })
      iDevice.count = +iDevice.count + 1;
      iDevice.frequency = message.message_at - iDevice.message_at;
      iDevice.data = message.data;
      iDevice.message_at = message.message_at;
    },

    // обработать сообщение
    handleMessage(message) {
      if (!message) {
        return;
      }
      // check if device is only one
      let ids = this.dlcList.filter(device => {
        return device.id === message.id;
      });

      let devIndex = -1;
      // если одно
      if (ids.length < 2) {
        devIndex = (this.dlcList.map(device => {
          return device.id;
        }).indexOf(message.id));
      } else {
        // device more than 1
        // find by dlc length
        devIndex = this.dlcList.indexOf(ids[ids.map(id => {
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
         * Устройство появ илось на линии впервые
         */
        this.pushDevice(message);
      }
    },


    saveFile: function () {
      const data = JSON.stringify(this.dlcList.map(device => {
        return {
          id: device.id,
          dlc: device.dlc,
          comment: ''
        }
      }))
      try {
        fs.writeFileSync('can-device.json', data, 'utf-8');
      } catch (e) {
        alert('Failed to save the file !');
      }
    },
    openFile: function () {
      // const fs = require('fs');
      // let jsonDB = fs.readFileSync('can-device.json', 'utf-8');

      // добавить в список прослушивания
      // _.each(JSON.parse(jsonDB), dev => {
      // this.pushDevice(dev);
      // });
    },
    writeToPort(message) {
      if (this.device)
        this.device.write(message);
    },

  },
  unmounted() {
    this.closePort();
  }
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
