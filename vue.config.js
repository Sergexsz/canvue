const { IgnorePlugin } = require('webpack')

module.exports = {
    pluginOptions: {
        plugins: [
            new IgnorePlugin({
                resourceRegExp: /serialport/
            })
        ],
        electronBuilder: {
            nodeIntegration: true,
            externals: [
                'serialport'
            ]
        }
    }
}