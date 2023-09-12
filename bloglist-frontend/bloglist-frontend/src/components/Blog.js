import { useState} from 'react'

const Blog = ({blog, handleLike, handleDelete, username}) => {
  const blogStyle = {
    paddingTop: 10,
    paddingLeft: 2,
    border: 'solid',
    borderWidth: 1,
    marginBottom: 5
  }
  const [detailsVisible, setDetailsVisible] = useState(false)

  const hideWhenVisible = { display: detailsVisible ? 'none' : '' }
  const showWhenVisible = { display: detailsVisible ? '' : 'none' }

  const detailsVisibility = () => {
    setDetailsVisible(!detailsVisible)
  }

  const updateLike = (event) => {
    event.preventDefault()
    if(blog.user){
      handleLike({
        user: blog.user.id,
        likes: blog.likes + 1,
        author: blog.author,
        title: blog.title,
        url: blog.url
      }, blog.id)
    }
    else{
      handleLike({
        likes: blog.likes + 1,
        author: blog.author,
        title: blog.title,
        url: blog.url
      }, blog.id)
    }
    

  }

  const deleteBlog = (event) => {
    event.preventDefault()
    handleDelete(blog.id)
    
  }

  
 
  //console.log(blog)
  return (
    <div style={blogStyle}>
      <div style={hideWhenVisible} className="defaultVisible">
        {blog.title} {blog.author}
        <button onClick={detailsVisibility}>view</button>
      </div>
      <div style={showWhenVisible} className="notDefaultVisible">
        {blog.title} {blog.author}
        <div>
          <a href={blog.url}>{blog.url}</a>
        </div>
        <div>
          {blog.likes} <button onClick={updateLike}>like</button>
        </div>
        <div>
          {blog.user && blog.user.username}
        </div>
        <div>
          {blog.user && blog.user.username === username && <button onClick={deleteBlog}>remove</button>}
        </div>
        <button onClick={detailsVisibility}>hide</button>
      </div>
    </div>  
  )
}
export default Blog