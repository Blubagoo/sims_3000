/**
 * @file InputContext.h
 * @brief Input context stack for modal input handling.
 *
 * Allows UI layers to push/pop input contexts for modal dialogs,
 * tool selection, and other state-dependent input handling.
 */

#ifndef SIMS3000_INPUT_INPUTCONTEXT_H
#define SIMS3000_INPUT_INPUTCONTEXT_H

#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sims3000 {

/**
 * @class InputContext
 * @brief A single input context that can consume or pass through events.
 */
class InputContext {
public:
    using EventHandler = std::function<bool(const SDL_Event&)>;

    /**
     * Create a named input context.
     * @param name Debug name for the context
     */
    explicit InputContext(const std::string& name);
    virtual ~InputContext() = default;

    /**
     * Get context name (for debugging).
     */
    const std::string& getName() const { return m_name; }

    /**
     * Process an input event.
     * @param event SDL event
     * @return true if event was consumed, false to pass through
     */
    virtual bool processEvent(const SDL_Event& event);

    /**
     * Set custom event handler.
     * @param handler Function to handle events
     */
    void setEventHandler(EventHandler handler);

    /**
     * Check if this context handles Escape key.
     */
    bool handlesEscape() const { return m_handlesEscape; }

    /**
     * Set whether this context handles Escape.
     */
    void setHandlesEscape(bool handles) { m_handlesEscape = handles; }

    /**
     * Check if this context is blocking (prevents input from reaching lower contexts).
     */
    bool isBlocking() const { return m_blocking; }

    /**
     * Set blocking mode.
     */
    void setBlocking(bool blocking) { m_blocking = blocking; }

protected:
    std::string m_name;
    EventHandler m_handler;
    bool m_handlesEscape = false;
    bool m_blocking = false;
};

/**
 * @class InputContextStack
 * @brief Manages a stack of input contexts.
 *
 * Events are processed from top to bottom until consumed.
 * The base context always exists and cannot be popped.
 */
class InputContextStack {
public:
    InputContextStack();
    ~InputContextStack() = default;

    /**
     * Push a new context onto the stack.
     * @param context Context to push (takes ownership)
     */
    void push(std::unique_ptr<InputContext> context);

    /**
     * Pop the top context from the stack.
     * Does nothing if only base context remains.
     * @return The popped context, or nullptr if base context
     */
    std::unique_ptr<InputContext> pop();

    /**
     * Pop all contexts except the base context.
     */
    void popAll();

    /**
     * Get the number of contexts (including base).
     */
    std::size_t size() const { return m_stack.size(); }

    /**
     * Check if there are contexts above the base.
     */
    bool hasModalContexts() const { return m_stack.size() > 1; }

    /**
     * Process an event through the context stack.
     * @param event SDL event
     * @return true if event was consumed
     */
    bool processEvent(const SDL_Event& event);

    /**
     * Get context names for debugging.
     * @return Vector of context names, top to bottom
     */
    std::vector<std::string> getContextNames() const;

    /**
     * Get the top context.
     */
    InputContext* top();
    const InputContext* top() const;

    /**
     * Get the base context.
     */
    InputContext* base();
    const InputContext* base() const;

private:
    std::vector<std::unique_ptr<InputContext>> m_stack;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_INPUTCONTEXT_H
