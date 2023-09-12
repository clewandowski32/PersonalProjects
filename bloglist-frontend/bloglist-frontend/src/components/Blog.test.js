import React from 'react'
import '@testing-library/jest-dom/extend-expect'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import Blog from './Blog'

test('default renders blog title and author', () => {
    const blog = {
        title: "A",
        author: "B",
        url: "C",
        likes: 10,
        user: {
            username: "D"
        }
    }

    const mockHandler = jest.fn()

    const { container } = render(
        <Blog blog={blog} handleLike={mockHandler} handleDelete={mockHandler} username={blog.user.username}/>
    )

    const div = container.querySelector('.defaultVisible')
    expect(div).not.toHaveStyle('display: none')
})

test('default does not render blog url and likes', async () => {
    const blog = {
        title: "A",
        author: "B",
        url: "C",
        likes: 10,
        user: {
            username: "D"
        }
    }

    const mockHandler = jest.fn()

    const { container } = render(
        <Blog blog={blog} handleLike={mockHandler} handleDelete={mockHandler} username={blog.user.username}/>
    )

    const user = userEvent.setup()
    const button = screen.getByText('view')
    await user.click(button)


    const div = container.querySelector('.notDefaultVisible')
    expect(div).not.toHaveStyle('display: none')
})

test('like button handles call correctly', async () => {
    const blog = {
        title: "A",
        author: "B",
        url: "C",
        likes: 10,
        user: {
            username: "D"
        }
    }

    const mockHandler = jest.fn()

    const { container } = render(
        <Blog blog={blog} handleLike={mockHandler} handleDelete={mockHandler} username={blog.user.username}/>
    )

    const user = userEvent.setup()
    const button = screen.getByText('view')
    await user.click(button)


    const likeButton = screen.getByText('like')
    await user.click(likeButton)
    await user.click(likeButton)

    expect(mockHandler.mock.calls).toHaveLength(2)
}) 