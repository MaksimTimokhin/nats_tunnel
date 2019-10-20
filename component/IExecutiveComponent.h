class IExecutiveComponent {
public:
    virtual ~IExecutiveComponent() = default;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;
};