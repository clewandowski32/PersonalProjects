const NewBlog = ({
    handleTitleChange,
    handleAuthorChange,
    handleUrlChange,
    newTitle,
    newAuthor,
    newUrl,
    addBlog
}) => {
    return(
    <div>
      <form onSubmit={addBlog}>
        <div>
          Title
          <input
              type="text"
              value={newTitle}
              name="Title"
              onChange={handleTitleChange}
              placeholder="enter title here"
            />
        </div>
        <div>
          Author
          <input
            type="text"
            value={newAuthor}
            name="Author"
            onChange={handleAuthorChange}
            placeholder="enter author here"
          />
        </div>
        <div>
          URL
          <input
            type="text"
            value={newUrl}
            name="URL"
            onChange={handleUrlChange}
            placeholder="enter url here"
          />
        </div>
        <button type="submit">add Blog</button>
      </form>
    </div>      
    )
}

export default NewBlog