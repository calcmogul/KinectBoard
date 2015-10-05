// =============================================================================
// File Name: ClientBase.inl
// Description: Base class for video stream providers
// Author: FRC Team 3512, Spartatroniks
// =============================================================================

template <class T>
void ClientBase<T>::setObject(T* object) {
    m_object = object;
}

template <class T>
void ClientBase<T>::setNewImageCallback(void (T::*newImageCbk)(uint8_t* buf, int bufsize)) {
    m_newImageCbk = newImageCbk;
}

template <class T>
void ClientBase<T>::setStartCallback(void (T::*startCbk)()) {
    m_startCbk = startCbk;
}

template <class T>
void ClientBase<T>::setStopCallback(void (T::*stopCbk)()) {
    m_stopCbk = stopCbk;
}

template <class T>
void ClientBase<T>::callNewImage(uint8_t* buf, int bufsize) {
    (m_object->*m_newImageCbk)(buf, bufsize);
}

template <class T>
void ClientBase<T>::callStart() {
    (m_object->*m_startCbk)();
}

template <class T>
void ClientBase<T>::callStop() {
    (m_object->*m_stopCbk)();
}
