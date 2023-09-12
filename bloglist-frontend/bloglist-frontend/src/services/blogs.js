import axios from 'axios'
const baseUrl = '/api/blogs'

let token = null

const setToken = newToken => {
  token = `Bearer ${newToken}`
}

const getAll = async () => {
  const config = {
    headers: { Authorization: token },
  }
  const response = await axios.get(baseUrl, config)
  return response.data
}

const create = async newObject => {
  const config = {
    headers: { Authorization: token },
  }

  const response = await axios.post(baseUrl, newObject, config)
  return response.data
}

const addLike = async (newObject, id) => {
  console.log(newObject)
  console.log(id)
  const config = {
    headers: {Authorization: token },
  }
  const url = `${baseUrl}/${id}`
  const response = await axios.put(url, newObject, config)
  return response.data
}

const deleteBlog = async (id) => {
  console.log(id)
  const config = {
    headers: {Authorization: token },
  }
  const url = `${baseUrl}/${id}`
  const response = await axios.delete(url, config)
  return response.data
}

export default { getAll, setToken, create, addLike, deleteBlog}