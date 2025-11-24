#pragma once

#include <RmlUi/Core.h>
#include <string>
#include <functional>
#include <memory>

namespace se {

class UIElement {
   public:
    UIElement(Rml::Element* element);
    virtual ~UIElement() = default;

    void SetText(const std::string& text);
    void SetProperty(const std::string& name, const std::string& value);
    void SetClass(const std::string& name, bool active);
    
    // Event handling
    using EventCallback = std::function<void()>;
    void AddEventListener(const std::string& event, EventCallback callback);

    Rml::Element* GetInternalElement() const { return element_; }

   private:
    Rml::Element* element_;
};

class Canvas {
   public:
    Canvas(const std::string& name);
    ~Canvas();

    static std::shared_ptr<Canvas> Create(const std::string& name);

    void LoadFromRML(const std::string& path);
    void Show();
    void Hide();
    
    std::shared_ptr<UIElement> GetElementById(const std::string& id);
    std::shared_ptr<UIElement> CreateElement(const std::string& tag);

    Rml::ElementDocument* GetDocument() const { return document_; }

   private:
    Rml::ElementDocument* document_ = nullptr;
    std::string name_;
};

}  // namespace se
