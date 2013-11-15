#ifndef Magnum_Platform_ScreenedApplication_h
#define Magnum_Platform_ScreenedApplication_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Magnum::Platform::BasicScreenedApplication
 */

#include <Containers/LinkedList.h>
#include <Magnum.h>

#include "Platform/Platform.h"

namespace Magnum { namespace Platform {

/**
@brief Base for applications with screen management

Manages list of screens and propagates events to them.

If exactly one application header is included, this class is also aliased to
`Platform::ScreenedApplication`.

Each @ref BasicScreen "Screen" specifies which set of events should be
propagated to it using @ref BasicScreen::setPropagatedEvents(). When
application gets an event, they are propagated to the screens:

-   @ref Sdl2Application::viewportEvent() "viewportEvent()" is propagated to
    all screens.
-   @ref Sdl2Application::drawEvent() "drawEvent()" is propagated in
    back-to-front order to screens which have @ref BasicScreen::PropagatedEvent::Draw
    enabled.
-   Input events (@ref Sdl2Application::keyPressEvent() "keyPressEvent()",
    @ref Sdl2Application::keyReleaseEvent() "keyReleaseEvent()",
    @ref Sdl2Application::mousePressEvent() "mousePressEvent()",
    @ref Sdl2Application::mouseReleaseEvent() "mouseReleaseEvent()"
    and @ref Sdl2Application::mouseMoveEvent() "mouseMoveEvent()")
    are propagated in front-to-back order to screens which have
    @ref BasicScreen::PropagatedEvent::Input enabled. If any screen sets the
    event as accepted, it is not propagated further.

Traversing through the list of screens is done like following:
@code
// front-to-back
for(Screen* s = app.frontScreen(); s; s = s->nextFartherScreen()) {
    // ...
}

// back-to-front
for(Screen* s = app.backScreen(); s; s = s->nextNearerScreen()) {
    // ...
}
@endcode

@section ScreenedApplication-explicit-specializations Explicit template specializations

The following specialization are explicitly compiled into each particular
`*Application` library. For other specializations you have to use
@ref ScreenedApplication.hpp implementation file to avoid linker errors. See
@ref compilation-speedup-hpp for more information.

-   @ref GlutApplication "BasicScreenedApplication<GlutApplication>"
-   @ref GlxApplication "BasicScreenedApplication<GlxApplication>"
-   @ref NaClApplication "BasicScreenedApplication<NaClApplication>"
-   @ref Sdl2Application "BasicScreenedApplication<Sdl2Application>"
-   @ref XEglApplication "BasicScreenedApplication<XEglApplication>"
*/
template<class Application> class BasicScreenedApplication: public Application, private Containers::LinkedList<BasicScreen<Application>> {
    friend class Containers::LinkedList<BasicScreen<Application>>;
    friend class Containers::LinkedListItem<BasicScreen<Application>, BasicScreenedApplication<Application>>;
    friend class BasicScreen<Application>;

    public:
        /** @copydoc Sdl2Application::Sdl2Application(const Arguments, const Configuration&) */
        explicit BasicScreenedApplication(const typename Application::Arguments& arguments, const typename Application::Configuration& configuration = Application::Configuration());

        /** @copydoc Sdl2Application::Sdl2Application(const Arguments&, std::nullptr_t) */
        #ifndef CORRADE_GCC45_COMPATIBILITY
        explicit BasicScreenedApplication(const typename Application::Arguments& arguments, std::nullptr_t);
        #else
        explicit BasicScreenedApplication(const typename Application::Arguments& arguments, void*);
        #endif

        /**
         * @brief Add screen to application
         * @return Reference to self (for method chaining)
         *
         * The new screen is added as backmost. If this is the first screen
         * added, @ref BasicScreen::focusEvent() is called. If not, neither
         * @ref BasicScreen::blurEvent() nor @ref BasicScreen::focusEvent() is
         * called (i.e. the screen default state is used).
         */
        BasicScreenedApplication<Application>& addScreen(BasicScreen<Application>& screen);

        /**
         * @brief Remove screen from application
         * @return Reference to self (for method chaining)
         *
         * The screen is blurred before removing. Deleting the object is left
         * up to the user.
         * @see @ref BasicScreen::blurEvent()
         */
        BasicScreenedApplication<Application>& removeScreen(BasicScreen<Application>& screen);

        /**
         * @brief Focus screen
         * @return Reference to self (for method chaining)
         *
         * Moves the screen to front. Previously focused screen is blurred and
         * this screen is focused.
         * @see @ref BasicScreen::blurEvent(), @ref BasicScreen::focusEvent()
         */
        BasicScreenedApplication<Application>& focusScreen(BasicScreen<Application>& screen);

        /**
         * @brief Front screen
         *
         * @see @ref BasicScreen::nextFartherScreen(), @ref BasicScreen::nextNearerScreen()
         */
        BasicScreen<Application>* frontScreen() {
            return Containers::LinkedList<BasicScreen<Application>>::first();
        }
        /** @overload */
        const BasicScreen<Application>* frontScreen() const {
            return Containers::LinkedList<BasicScreen<Application>>::first();
        }

        /**
         * @brief Back screen
         *
         * @see @ref BasicScreen::nextFartherScreen(), @ref BasicScreen::nextNearerScreen()
         */
        BasicScreen<Application>* backScreen() {
            return Containers::LinkedList<BasicScreen<Application>>::last();
        }
        /** @overload */
        const BasicScreen<Application>* backScreen() const {
            return Containers::LinkedList<BasicScreen<Application>>::last();
        }

    protected:
        /* Nobody will need to have (and delete) ScreenedApplication*, thus
           this is faster than public pure virtual destructor */
        ~BasicScreenedApplication();

        /**
         * @brief Global viewport event
         *
         * Called when window size changes, *before* all screens'
         * @ref BasicScreen::viewportEvent() "viewportEvent()". You should at
         * least pass the new size to @ref DefaultFramebuffer::setViewport().
         *
         * Note that this function might not get called at all if the window
         * size doesn't change. You are responsible for configuring the initial
         * state yourself, viewport of default framebuffer can be retrieved
         * from @ref DefaultFramebuffer::viewport().
         */
        virtual void globalViewportEvent(const Vector2i& size) = 0;

        /**
         * @brief Draw event
         *
         * Called *after* all screens' @ref BasicScreen::drawEvent() "drawEvent()".
         * You should call at least @ref Sdl2Application::swapBuffers() "swapBuffers()".
         * If you want to draw immediately again, call also
         * @ref Sdl2Application::redraw() "redraw()".
         */
        virtual void globalDrawEvent() = 0;

    private:
        /* The user is supposed to override only globalViewportEvent() and
           globalDrawEvent(), these implementations are dispatching the events
           to attached screens. */
        void viewportEvent(const Vector2i& size) override final;
        void drawEvent() override final;
        void keyPressEvent(typename Application::KeyEvent& event) override final;
        void keyReleaseEvent(typename Application::KeyEvent& event) override final;
        void mousePressEvent(typename Application::MouseEvent& event) override final;
        void mouseReleaseEvent(typename Application::MouseEvent& event) override final;
        void mouseMoveEvent(typename Application::MouseMoveEvent& event) override final;
};

}}

#endif