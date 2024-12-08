/**
 * We'll load jQuery and the Bootstrap jQuery plugin which provides support
 * for JavaScript based Bootstrap features such as modals and tabs. This
 * code may be modified to fit the specific needs of your application.
 */

/**
 * Element PLUS
 */
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
// import './styles/element.scss'
import ru from 'element-plus/dist/locale/ru'
import 'dayjs/locale/ru'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'

import {createApp} from 'vue'
import App from './App.vue'
import {Quasar} from 'quasar'
import quasarUserOptions from './quasar-user-options'

import mitt from 'mitt';
const emitter = mitt();


const app = createApp(App);
app.config.globalProperties.emitter = emitter;
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
    app.component(key, component)
}
app.use(ElementPlus, {locale: ru})
app.use(Quasar, quasarUserOptions)
app.mount('#app')