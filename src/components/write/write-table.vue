<template>
  <div class="bg-white p-2 shadow rounded mb-2">
    <input type="file" @change="fileSelect" ref="file" class="d-none">

    <div class="d-flex w-100 text-start mb-2 justify-content-between align-items-center">
      <div class="fw-bold w-100 text-start w-25 me-3">Список собственных пакетов
        ({{ outStock.length }})
      </div>
      <div class="w-75 text-end">
        <button class="btn btn-outline-info btn-sm me-2" @click="saveToLocal">Сохранить в файл</button>
        <button class="btn btn-outline-info btn-sm me-2" @click="$refs.file.click()">Загрузить из файла</button>
        <button class="btn btn-outline-info btn-sm me-2" @click="pushDevices">В список устройств</button>
        <button class="btn btn-success btn-sm" @click="push">Добавить</button>
      </div>
    </div>
    <div class="overflow-auto" style="max-height: 350px">

      <table class="table table-sm table-striped table-bordered">
        <thead>
        <tr class="tr fw-bold small text-muted text-uppercase sticky-top bg-white" style="top:0">
          <td style="width:120px">id</td>
          <td style="width:50px">dlc</td>
          <td colspan="8">data</td>
          <td>comment</td>
          <td style="width:80px">period</td>
          <td style="width:80px">count</td>
          <td>actions</td>
        </tr>
        </thead>
        <tbody>
        <write-item
            v-for="(out,index) in outStock"
            :key="index"
            v-model="outStock"
            :value="out"
            @input="saveFile"
            @delete="removeItem(out)"
            @written="writeMessage(out)"></write-item>
        </tbody>
      </table>
    </div>
    <button class="btn btn-success w-100 my-1" @click="push">Добавить еще</button>
  </div>
</template>

<script>
import {ElMessage} from "element-plus";
import WriteItem from "./write-item"

import Store from 'electron-store'
import _ from "lodash";

const eStore = new Store();
const fileName = 'canWriteTable';

export default {
  name: "write-table",
  components: {WriteItem},
  emits: ['written', 'pushDevice'],
  data() {
    return {
      outStock: [
        // {
        //   id: "8EF87310",
        //   dlc: 2,
        //   count: 0,
        //   period: 0,
        //   data: ["F8", "0"],
        //   comment: ''
        // }
      ]
    }
  },
  mounted() {
    this.openFile();
    this.emitter.on("save-to-write-table", this.pushPreData);
  },
  methods: {
    // сохранить талицу в файл
    saveToLocal() {
      let data = JSON.stringify(this.outStock);
      let file = new Blob([data], {type: "application/json"});
      let a = document.createElement("a"),
          url = URL.createObjectURL(file);
      a.href = url;
      a.download = "can-write-table.json";
      document.body.appendChild(a);
      a.click();
      setTimeout(function () {
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
      }, 0);
    },
    // скачать таблицу в файл
    fileSelect($event) {
      let file = $event.target.files[0];
      if (file && file.type === "application/json") {
        let reader = new FileReader();
        if (reader) {
          let self = this;
          reader.readAsText(file, "UTF-8");
          reader.onload = function (evt) {
            self.outStock = JSON.parse(evt.target.result);
            ElMessage.success('Таблица данных обновлена')
          }
        }
      } else {
        ElMessage.error('Неверный файл')
      }
      this.$refs.file.value = '';
    },
    writeMessage(out) {
      const device = parseInt(out.id, 16); // например "0x200" -> 512
      const dlc = parseInt(out.dlc, 10);   // длина 0..8
      const data = out.data.map(b => parseInt(b, 16)); // массив байтов

      // this.$emit("written", 'w' + device + '.' + dlc + '.' + data.join('.'));
     // out.count = +out.count + 1;

      const buffer = Buffer.alloc(15);
      buffer[0] = 0xAA;

      // Записываем ID (4 байта, Big Endian)
      buffer.writeUInt32BE(device >>> 0, 1);

      buffer[5] = dlc;

      for (let i = 0; i < 8; i++) {
        buffer[6 + i] = i < data.length ? data[i] : 0x00;
      }

      buffer[14] = 0x55;

      // Отправка по порту
      this.$emit("written", buffer);
      out.count = +out.count + 1;

    },

    pushPreData(item) {
      this.outStock.push(
          {
            id: item.id,
            dlc: item.dlc,
            count: 0,
            period: 0,
            data: _.map(item.data, v => {
              return v.value
            }),
            comment: ''
          }
      );
      this.saveFile();
    },
    push() {
      this.outStock.push(
          {
            id: "",
            dlc: 0,
            count: 0,
            period: 0,
            data: [],
            comment: ''
          }
      );
      this.saveFile();
    },
    removeItem(out) {
      this.outStock.splice(this.outStock.indexOf(out), 1);
      this.saveFile();
    },
    saveFile: function () {
      try {
        eStore.set(fileName, this.outStock);
        ElMessage.success('Сохранено');
      } catch (e) {
        ElMessage.error('Не удалось сохранить в файл');
      }
    },
    pushDevices() {
      // добавить в список прослушивания
      _.each(this.outStock, dev => {
        this.$emit('pushDevice', dev);
      });
    },
    openFile: function () {
      if (eStore.get(fileName))
        this.outStock = eStore.get(fileName);
    },


  }
}
</script>

<style scoped>

</style>