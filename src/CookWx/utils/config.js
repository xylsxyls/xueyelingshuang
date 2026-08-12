const API_HOST = '175.24.198.162'
const API_PORT = 5207
const API_PROTOCOL = 'http'

const LOCAL_API_HOST = '127.0.0.1'
const LOCAL_API_PORT = 5207

const USE_LOCAL_SERVER = false

const activeHost = USE_LOCAL_SERVER ? LOCAL_API_HOST : API_HOST
const activePort = USE_LOCAL_SERVER ? LOCAL_API_PORT : API_PORT

module.exports = {
  API_HOST,
  API_PORT,
  API_PROTOCOL,
  LOCAL_API_HOST,
  LOCAL_API_PORT,
  USE_LOCAL_SERVER,
  BASE_URL: `${API_PROTOCOL}://${activeHost}:${activePort}`
}