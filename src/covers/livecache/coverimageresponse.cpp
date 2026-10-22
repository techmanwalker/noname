#include "coverstorage.hpp"

cover_image_response::cover_image_response(std::shared_ptr<cover_response_state> state)
    : m_state(std::move(state))
{
}

QQuickTextureFactory *
cover_image_response::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_state->image);
}

void
cover_image_response::cancel()
{
    m_state->canceled.storeRelaxed(1);
}

cover_resolve_task::cover_resolve_task(covers::live::cover_storage *storage,
                                       QString id, QSize size,
                                       std::shared_ptr<cover_response_state> state,
                                       QPointer<cover_image_response> response,
                                       mailbox_ptr mailbox)
    : m_storage(storage), m_id(std::move(id)), m_size(size),
      m_state(std::move(state)), m_response(std::move(response)),
      m_mailbox(std::move(mailbox))
{
}

void
cover_resolve_task::run()
{
    if (m_storage && !m_state->canceled.loadRelaxed()) {
        m_state->image = m_storage->resolve_blocking(m_id, m_size);
    }

    // Hand off to the response's thread. The response is only dereferenced
    // over there, where its deleteLater() is serialized with this call.
    // finished() is emitted even when cancelled so Qt can clean up.
    QMetaObject::invokeMethod(m_mailbox.get(), [response = m_response] {
        if (response)
            response->deliver();
    }, Qt::QueuedConnection);
}