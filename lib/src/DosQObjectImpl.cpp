#include "DOtherSide/DosQObjectImpl.h"
#include "DOtherSide/DosQMetaObject.h"
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <QtCore/QDebug>

namespace DOS
{

DosQObjectImpl::DosQObjectImpl(QObject* parent,
                               OnMetaObject onMetaObject,
                               OnSlotExecuted onSlotExecuted)
    : m_parent(std::move(parent))
    , m_onMetaObject(std::move(onMetaObject))
    , m_onSlotExecuted(std::move(onSlotExecuted))
{
    *static_cast<QMetaObject*>(this) = *m_onMetaObject()->data()->metaObject();
    QObjectPrivate::get(parent)->metaObject = this;
}

bool DosQObjectImpl::emitSignal(const QString &name, const std::vector<QVariant> &args)
{
    const QMetaMethod method = dosMetaObject()->signal(name);
    if (!method.isValid())
        return false;

    Q_ASSERT(name.toUtf8() == method.name());

    std::vector<void*> arguments(args.size() + 1, nullptr);
    arguments.front() = nullptr;
    auto func = [](const QVariant& arg) -> void* { return (void*)(&arg); };
    std::transform(args.begin(), args.end(), arguments.begin() + 1, func);
    QMetaObject::activate(m_parent, method.methodIndex(), arguments.data());
    return true;
}

int DosQObjectImpl::metaCall(QMetaObject::Call callType, int index, void **args)
{
    switch (callType)
    {
    case QMetaObject::InvokeMetaMethod:
        executeSlot(index, args);
        break;
    case QMetaObject::ReadProperty:
        readProperty(index, args);
        break;
    case QMetaObject::WriteProperty:
        writeProperty(index, args);
        break;
    }

    return -1;
}

std::shared_ptr<const IDosQMetaObject> DosQObjectImpl::dosMetaObject() const
{
    static std::shared_ptr<const IDosQMetaObject> result = m_onMetaObject()->data();
    return result;
}

bool DosQObjectImpl::executeSlot(int index, void **args)
{
    const QMetaMethod method = this->method(index);
    if (!method.isValid()) {
        qDebug() << "C++: executeSlot: invalid method";
        return false;
    }
    executeSlot(method, args);
}

bool DosQObjectImpl::executeSlot(const QMetaMethod &method, void **args)
{
    Q_ASSERT(method.isValid());

    const bool hasReturnType = method.returnType() != QMetaType::Void;

    std::vector<QVariant> arguments;
    arguments.reserve(method.parameterCount());
    for (int i = 0; i < method.parameterCount(); ++i) {
        QVariant argument(method.parameterType(i), args[i+1]);
        arguments.emplace_back(std::move(argument));
    }

    QVariant result = m_onSlotExecuted(method.name(), arguments); // Execute method

    if (hasReturnType && result.isValid()) {
        QMetaType::construct(method.returnType(), args[0], result.constData());
    }

    return true;
}

bool DosQObjectImpl::readProperty(int index, void **args)
{
    const QMetaProperty property = this->property(index);
    if (!property.isValid() || !property.isReadable())
        return false;
    QMetaMethod method = dosMetaObject()->readSlot(property.name());
    if (!method.isValid()) {
        qDebug() << "C++: readProperty: invalid read method for property " << property.name();
        return false;
    }
    return executeSlot(method, args);
}

bool DosQObjectImpl::writeProperty(int index, void **args)
{
    const QMetaProperty property = this->property(index);
    if (!property.isValid() || !property.isWritable())
        return false;
    QMetaMethod method = dosMetaObject()->writeSlot(property.name());
    if (!method.isValid()) {
        qDebug() << "C++: writeProperty: invalid write method for property " << property.name();
        return false;
    }
    return executeSlot(method, args);
}

}
