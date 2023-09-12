import { useState, useEffect} from 'react'
import Notification from './components/Notification'
import Error from './components/Error'
import Blog from './components/Blog'
import blogService from './services/blogs'
import loginService from './services/login'
import NewBlog from './components/newBlogForm'
import Togglable from './components/Togglable'

const App = () => {
  const [blogs, setBlogs] = useState([])
  const [username, setUsername] = useState('') 
  const [password, setPassword] = useState('') 
  const [user, setUser] = useState(null)
  const [newTitle, setNewTitle] = useState('')
  const [newAuthor, setNewAuthor] = useState('')
  const [newUrl, setNewUrl] = useState('')
  const [notificationMessage, setNotificationMessage] = useState(null)
  const [errorMessage, setErrorMessage] = useState(null)
  //const [newBlogVisible, setNewBlogVisible] = useState(null)

  // const blogsFormRef = useRef()

  const handleLike = (blogObject, blogId) => {

      blogService
        .addLike(blogObject, blogId)
        .then(
          blogService
            .getAll()
            .then(
              blogs => {
                setBlogs( blogs )
                blogs.sort((a, b) => {
                const likesA = a.likes
                const likesB = b.likes
                if(likesA > likesB){
                  return -1
                }
                if(likesA < likesB){
                  return 1
                }
          
                return 0
              })}
              
        ))

  }

  const handleDelete = (blogId) => {
    blogService
      .deleteBlog(blogId)
      .then(
        blogService
          .getAll()
          .then(
            blogs => {
              setBlogs( blogs )
              blogs.sort((a, b) => {
              const likesA = a.likes
              const likesB = b.likes
              if(likesA > likesB){
                return -1
              }
              if(likesA < likesB){
                return 1
              }
        
              return 0
            })}            
      ))
  }

  const addBlog = async (event) => {
    event.preventDefault()

    //blogsFormRef.current.toggleVisibility()

    try {

      const blogObject = {
        title: newTitle,
        author: newAuthor,
        url: newUrl
      }

      const newBlog = await blogService
      .create(blogObject)

      blogService.getAll().then(blogs =>
        setBlogs( blogs )
      )
      setNewTitle('')
      setNewAuthor('')
      setNewUrl('')

      setNotificationMessage(`added ${newBlog.title} by ${newBlog.author} successfully`)
      setTimeout(() => {
        setNotificationMessage(null)
        }, 5000)
    } catch (execption){
      setErrorMessage('unsuccessfully added new blog')
      setTimeout(() => {
      setErrorMessage(null)
      }, 5000)
    }
  }

  useEffect(() => {
    blogService.getAll().then(blogs =>
      setBlogs( blogs )
    )  
  }, [])

  useEffect(() => {
    blogs.sort((a, b) => {
      const likesA = a.likes
      const likesB = b.likes
      if(likesA > likesB){
        return -1
      }
      if(likesA < likesB){
        return 1
      }

      return 0
    })
  })

  //hook for checking if user is stored in local storage each time
  //page is reloaded
  useEffect(() => {
    const loggedUserJSON = window.localStorage.getItem('loggedBlogappUser')
    if (loggedUserJSON) {
      const user = JSON.parse(loggedUserJSON)
      setUser(user)
      blogService.setToken(user.token)
    }
  }, [])


  const handleLogin = async (event) => {
    event.preventDefault()
    
    try {
      const user = await loginService.login({
        username, password,
      })
      blogService.setToken(user.token)
      window.localStorage.setItem(
        'loggedBlogappUser', JSON.stringify(user)
      ) 
      //allows user login to persist after page refreshes by saving to local storage
      setUser(user)
      setUsername('')
      setPassword('')
    } catch (exception) {
      // setErrorMessage('Wrong credentials')
      // setTimeout(() => {
      //   setErrorMessage(null)
      // }, 5000)
      console.log('error logging in')
    }
  }

  const loginForm = () => (
    <div>
      <h2>Log in to application</h2>
      <form onSubmit={handleLogin}>
        <div>
          username
            <input
            type="text"
            value={username}
            name="Username"
            onChange={({ target }) => setUsername(target.value)}
          />
        </div>
        <div>
          password
            <input
            type="password"
            value={password}
            name="Password"
            onChange={({ target }) => setPassword(target.value)}
          />
        </div>
        <button type="submit">login</button>
      </form> 
    </div>     
  )
//   const blogsDisplay = () => {
//     return (
//     <div>
//       {user.name} is logged in 
//       <form onSubmit={() => window.localStorage.removeItem('loggedBlogappUser')}>
//         <button type="submit">logout</button>
//       </form>
//       <Togglable buttonLabel='new blog' ref={blogsFormRef}>
//           <NewBlog
//             newTitle={newTitle}
//             newAuthor={newAuthor}
//             newUrl={newUrl}
//             handleTitleChange={({ target }) => setNewTitle(target.value)}
//             handleAuthorChange={({ target }) => setNewAuthor(target.value)}
//             handleUrlChange={({ target }) => setNewUrl(target.value)}
//             addBlog={addBlog}
//           />
//         </Togglable>
//       {blogs.map(b => <Blog key={b.id} blog={b}/>)}
//     </div>
//   )
// }

  if (user === null){
    return (
      <div>
        {loginForm()}
      </div>
    )
  }
  return (
    <div>
      <Notification message={notificationMessage}/>
      <Error message={errorMessage}/>
      <div>
        {user.name} logged in successfully
        <form onSubmit={() => window.localStorage.removeItem('loggedBlogappUser')}>
         <button type="submit">logout</button>
       </form>
      </div>
      <h2>blogs</h2>
      <Togglable buttonLabel="new blog">
        <NewBlog
              newTitle={newTitle}
              newAuthor={newAuthor}
              newUrl={newUrl}
              handleTitleChange={({ target }) => setNewTitle(target.value)}
              handleAuthorChange={({ target }) => setNewAuthor(target.value)}
              handleUrlChange={({ target }) => setNewUrl(target.value)}
              addBlog={addBlog}
            />
      </Togglable>
      {blogs.map(blog =>
        {
          return(
            <Blog key={blog.id} blog={blog} handleLike={handleLike} handleDelete={handleDelete} username={user.username}/>
          )
        }      
      )}
    </div>
  )

  // return(
  //   <div>
  //     <Notification message={notificationMessage}/>
  //     {user === null && loginForm()}
  //     {user !== null && blogsDisplay()}
      
  //   </div>
  // )
}

export default App