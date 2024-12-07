<template>
  <div class="div shadow p-2 bg-white rounded mb-4">
    <div class="d-flex justify-content-between">
      <div class="fw-bold">Устройства на линии</div>
      <div class="">
        <button class="btn" @click="$emit('clear')">Очистить список</button>
      </div>
    </div>
    <table class="table table-striped table-sm">
      <thead class="sticky-top bg-white">
      <tr class="fw-bold small text-uppercase text-muted">
        <td style="width:50px">ID</td>

        <td style="width:50px" class="text-center text-info">DLC</td>
        <td colspan="8" style="text-align: center">Данные</td>
        <td style="width:60px">Кол-во</td>
        <td style="width:80px">Частота</td>
        <td>Название</td>
      </tr>
      </thead>
      <tbody>
      <table-inc-device
          v-model="devicesData"
          :list="devicesData"
          @input="saveFile"
          :key="device"
          v-for="device in dlcList"
          :item="device"></table-inc-device>
      </tbody>
    </table>
  </div>
</template>

<script>
import TableIncDevice from "@/components/incoming/table-inc-device";
import fs from 'fs'
import {ElMessage} from "element-plus";

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
    this.openFile()
  },
  methods: {
    openFile: function () {
      let jsonDB = fs.readFileSync('can-device.json', 'utf-8');
      this.devicesData = JSON.parse(jsonDB);
    },
    saveFile: function () {
      const data = JSON.stringify(this.devicesData)
      try {
        fs.writeFileSync('can-device.json', data, 'utf-8');
        // ElMessage.success('удалось сохранить в файл')
      } catch (e) {
        ElMessage.error('Не удалось сохранить в файл')
      }
    },
  }
}
</script>

<style scoped>

</style>