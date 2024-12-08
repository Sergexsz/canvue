<template>
  <div class="div shadow p-2 bg-white rounded mb-4">
    <input ref="file" type="file" @change="fileSelect" class="d-none">
    <div class="d-flex justify-content-between mb-2">
      <div class="fw-bold">Устройства на линии ({{dlcList.length}})</div>
      <div class="">
        <!--        <button class="btn" @click="clearFile">clearFile</button>-->
        <!--        <button class="btn" @click="openFile">openFile</button>-->
        <button class="btn btn-sm me-2 btn-outline-info" @click="saveToLocal">Сохранить в файл</button>
        <button class="btn btn-sm me-2 btn-outline-info" @click="$refs.file.click()">Загрузить из файла</button>
        <button class="btn btn-sm" @click="$emit('clear')">Очистить список</button>
      </div>
    </div>
    <div class="overflow-auto" style="max-height: 350px">

      <table class="table table-striped table-sm">
        <thead class="sticky-top bg-white">
        <tr class="fw-bold small text-uppercase text-muted">
          <td style="width:50px">ID</td>

          <td style="width:50px" class="text-center text-info">DLC</td>
          <td colspan="8" style="text-align: center">Данные</td>
          <td style="width:60px">Кол-во</td>
          <td style="width:80px">Частота</td>
          <td>Название</td>
          <td></td>
        </tr>
        </thead>
        <tbody>
        <table-inc-device
            @createdDevice="checkAndPushStore"
            v-model="devicesData"
            :list="devicesData"
            @input="saveFile"
            :key="device"
            v-for="device in dlcList"
            :item="device"></table-inc-device>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script>
import TableIncDevice from "@/components/incoming/table-inc-device";
import {ElMessage} from "element-plus";
import _ from 'lodash'
import Store from 'electron-store'

const eStore = new Store();
const fileName = 'canDeviceTable';

export default {
  name: "inc-table",
  components: {TableIncDevice},
  props: ['dlcList'],
  emits: ['clear'],
  data() {
    return {
      devicesData: []
    }
  },
  mounted() {
    this.openFile();
  },
  methods: {
    // сохранить талицу в файл
    saveToLocal() {
      let data = JSON.stringify(this.devicesData);
      let file = new Blob([data], {type: "application/json"});
      let a = document.createElement("a"),
          url = URL.createObjectURL(file);
      a.href = url;
      a.download = "can-devices-table.json";
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
            self.devicesData = JSON.parse(evt.target.result);
            ElMessage.success('Таблица данных обновлена')
          }
        }
      } else {
        ElMessage.error('Неверный файл')
      }
      this.$refs.file.value = '';
    },
    // добавить новое в банк
    checkAndPushStore(device) {
      if (_.findIndex(this.devicesData, {id: device.id}) === -1) {
        // console.log('new device '+ device.id + ' pushed to store');
        this.devicesData.push({
          id: device.id,
          dlc: device.dlc,
          comment: ''
        });
      }
    },
    clearFile() {
      eStore.delete(fileName);
    },
    openFile() {
      if (eStore.get(fileName))
        this.devicesData = eStore.get(fileName);
    },
    saveFile() {
      try {
        eStore.set(fileName, this.devicesData);
        // ElMessage.success('удалось сохранить в файл')
      } catch (e) {
        ElMessage.error('Не удалось сохранить в файл', e)
      }
    },
  }
}
</script>

<style scoped>

</style>