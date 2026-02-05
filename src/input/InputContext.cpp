/**
 * @file InputContext.cpp
 * @brief Input context stack implementation.
 */

#include "sims3000/input/InputContext.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

// InputContext

InputContext::InputContext(const std::string& name)
    : m_name(name)
{}

bool InputContext::processEvent(const SDL_Event& event) {
    if (m_handler) {
        return m_handler(event);
    }
    return false;
}

void InputContext::setEventHandler(EventHandler handler) {
    m_handler = std::move(handler);
}

// InputContextStack

InputContextStack::InputContextStack() {
    // Create base context that always exists
    auto baseContext = std::make_unique<InputContext>("base");
    m_stack.push_back(std::move(baseContext));
}

void InputContextStack::push(std::unique_ptr<InputContext> context) {
    if (context) {
        SDL_Log("InputContext: Pushing '%s' (stack depth: %zu)",
                context->getName().c_str(), m_stack.size() + 1);
        m_stack.push_back(std::move(context));
    }
}

std::unique_ptr<InputContext> InputContextStack::pop() {
    if (m_stack.size() <= 1) {
        // Cannot pop base context
        return nullptr;
    }

    auto context = std::move(m_stack.back());
    m_stack.pop_back();
    SDL_Log("InputContext: Popped '%s' (stack depth: %zu)",
            context->getName().c_str(), m_stack.size());
    return context;
}

void InputContextStack::popAll() {
    while (m_stack.size() > 1) {
        pop();
    }
}

bool InputContextStack::processEvent(const SDL_Event& event) {
    // Process from top to bottom
    for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it) {
        if ((*it)->processEvent(event)) {
            return true; // Event consumed
        }
        if ((*it)->isBlocking()) {
            return false; // Blocking context stops propagation
        }
    }
    return false;
}

std::vector<std::string> InputContextStack::getContextNames() const {
    std::vector<std::string> names;
    names.reserve(m_stack.size());
    for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it) {
        names.push_back((*it)->getName());
    }
    return names;
}

InputContext* InputContextStack::top() {
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

const InputContext* InputContextStack::top() const {
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

InputContext* InputContextStack::base() {
    return m_stack.empty() ? nullptr : m_stack.front().get();
}

const InputContext* InputContextStack::base() const {
    return m_stack.empty() ? nullptr : m_stack.front().get();
}

} // namespace sims3000
