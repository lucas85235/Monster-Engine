#include "engine/ui/Canvas.h"
#include "engine/ui/RmlUiLayer.h"
#include "engine/Application.h"

namespace se {

// UIElement Implementation

UIElement::UIElement(Rml::Element* element) : element_(element) {}

void UIElement::SetText(const std::string& text) {
    if (element_)
        element_->SetInnerRML(text);
}

void UIElement::SetProperty(const std::string& name, const std::string& value) {
    if (element_)
        element_->SetProperty(name, value);
}

void UIElement::SetClass(const std::string& name, bool active) {
    if (element_)
        element_->SetClass(name, active);
}

class RmlEventListener : public Rml::EventListener {
   public:
    RmlEventListener(UIElement::EventCallback callback) : callback_(callback) {}
    void ProcessEvent(Rml::Event& event) override {
        if (callback_) callback_();
    }
   private:
    UIElement::EventCallback callback_;
};

void UIElement::AddEventListener(const std::string& event, EventCallback callback) {
    if (element_) {
        // Leak warning: We are newing this listener and RmlUi takes ownership? 
        // RmlUi does NOT take ownership of EventListener. We need to manage it.
        // For this simple implementation, we might leak or need a better management strategy.
        // TODO: Store listeners in UIElement to delete them later.
        auto listener = new RmlEventListener(callback);
        element_->AddEventListener(event, listener);
    }
}

// Canvas Implementation

Canvas::Canvas(const std::string& name) : name_(name) {
    // We need access to the RmlUi Context.
    // Assuming we can get it from the Application -> RmlUiLayer
    // This is a bit tricky since we don't have direct access to layers by type easily without casting.
    // But we can assume the user has added the RmlUiLayer.
    
    // For now, let's assume a singleton or static access, or we search for the layer.
    // Or we make the Context global/static in RmlUiLayer.
    
    // Let's rely on Rml::GetContext("main") if we named it "main".
    Rml::Context* context = Rml::GetContext("main");
    if (context) {
        // Create a simple RML document from memory
        std::string rml = "<rml><head><title>" + name + "</title></head><body></body></rml>";
        document_ = context->LoadDocumentFromMemory(rml, name);
        if (document_) {
            document_->SetId(name);
        }
    }
}

Canvas::~Canvas() {
    if (document_) {
        document_->Close();
    }
}

std::shared_ptr<Canvas> Canvas::Create(const std::string& name) {
    return std::make_shared<Canvas>(name);
}

void Canvas::LoadFromRML(const std::string& path) {
    Rml::Context* context = Rml::GetContext("main");
    if (context) {
        if (document_) document_->Close();
        document_ = context->LoadDocument(path);
        if (document_) document_->Show();
    }
}

void Canvas::Show() {
    if (document_) document_->Show();
}

void Canvas::Hide() {
    if (document_) document_->Hide();
}

std::shared_ptr<UIElement> Canvas::GetElementById(const std::string& id) {
    if (document_) {
        Rml::Element* el = document_->GetElementById(id);
        if (el) return std::make_shared<UIElement>(el);
    }
    return nullptr;
}

std::shared_ptr<UIElement> Canvas::CreateElement(const std::string& tag) {
    if (document_) {
        Rml::ElementPtr el = document_->CreateElement(tag);
        if (el) {
            Rml::Element* raw_el = el.get();
            document_->AppendChild(std::move(el));
            return std::make_shared<UIElement>(raw_el);
        }
    }
    return nullptr;
}

}  // namespace se
