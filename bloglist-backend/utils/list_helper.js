const dummy = (blogs) => {
    return 1
}

const totalLikes = (blogs) => {
    const reducer = (sum, item) => {
        return sum + item.likes
    }
    return blogs.reduce(reducer, 0)
}

const favoriteBlog = (blogs) => {
    const favorite = blogs.reduce((max, current) => {
        return current.likes > max.likes ? current : max
    })
    return favorite
}

module.exports = {
    dummy, totalLikes, favoriteBlog
}