import React from 'react'
import { render, screen } from '@testing-library/react'
import '@testing-library/jest-dom/extend-expect'
import NewBlog from './newBlogForm'
import userEvent from '@testing-library/user-event'

test('<NewBlog /> updates parent state and calls onSubmit', async () => {
  const addBlog = jest.fn()
  const user = userEvent.setup()

  render(<NewBlog addBlog={addBlog}/>)

  const inputTitle = screen.getByPlaceholderText('enter title here')
  const inputAuthor = screen.getByPlaceholderText('enter author here')
  const inputUrl = screen.getByPlaceholderText('enter url here')

  const sendButton = screen.getByText('add Blog')

  await user.type(inputTitle, 'Testing')
  await user.type(inputAuthor, 'Tester')
  await user.type(inputUrl, 'test.com')
  await user.click(sendButton)

  expect(addBlog.mock.calls).toHaveLength(1)
  expect(addBlog.mock.calls[0][0].content).toBe('Testing')
  expect(addBlog.mock.calls[0][1].content).toBe('Tester')
  expect(addBlog.mock.calls[0][2].content).toBe('test.com')
})